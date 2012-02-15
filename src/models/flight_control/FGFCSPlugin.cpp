/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Module:       FGFCSPlugin.cpp
 Author:       Matthew Gong
 Date started: 3/2008

 ------------- Copyright (C) 2008 Matthew Gong -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU Lesser General Public License can also be found on
 the world wide web at http://www.gnu.org.

PLUGINAL DESCRIPTION
--------------------------------------------------------------------------------

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
COMMENTS, REFERENCES,  and NOTES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#if USE_FGFCSPLUGIN

#include "FGFCSPlugin.h"

#include <ltdl.h>
#include <string>
#include <iostream>

using namespace std;

extern "C"
{
    typedef int (*PlugInFunc_ptr) (int stage, unsigned input_num, const double inputs[], unsigned output_num, double outputs[], double states[]);

    int plugin_run(int stage, unsigned input_num, const double inputs[], unsigned output_num, double outputs[], double states[])
    {
        if (output_num > 0u)
        {
            double & output = outputs[0];

            output = 0;

            for (unsigned i=0u; i < input_num; ++i)
            {
                output *=  inputs[i];
            }
        }
        return 0;
    }

}


//extern PlugInFunc_ptr run;

namespace JSBSim
{

static const char *IdSrc = "$Id: FGFCSPlugin.cpp $";
static const char *IdHdr = ID_FCSPLUGIN;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS IMPLEMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
struct Plugin
{
    Plugin();
    ~Plugin();

    enum Type
    {
        DLL_FUNC,
        UNDEFINED
    } type;

    string name;

    lt_dlhandle FunctionLib;
    PlugInFunc_ptr plugin_main;

    int result;
};

Plugin::Plugin() :
        FunctionLib(NULL),
        plugin_main(NULL),
        result(0)
{
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Plugin::~Plugin()
{
}


typedef vector<Plugin> PluginList;
typedef PluginList::iterator PluginListIter;
typedef PluginList::const_iterator PluginListCIter;

unsigned g_ltdlCount=0u;

PluginList g_plugin_list(256);
string PluginDir="plugins";

FGFCSPlugin::FGFCSPlugin(FGFCS* fcs, Element* element) : FGFCSComponent(fcs, element),is_Ok(false), use_builtin(false)
{
    input_size = InputNodes.size();
	output_size = OutputNodes.size();
	outputs = new double[output_size];

    Element *coeff_element = element->FindElement("coeff");
    while (coeff_element)
    {
        string coeff = coeff_element->GetDataLine();
        if (coeff[0] == '-')
        {
            CoeffSigns.push_back(-1.0);
            coeff.erase(0,1);
        }
        else
        {
            CoeffSigns.push_back( 1.0);
        }

        if (coeff.find_first_not_of("0123456789. ",0) != coeff.npos)
        {
            FGPropertyManager *tmp = PropertyManager->GetNode(coeff);
            if (tmp)
            {
                CoeffNodes.push_back( tmp );
                CoeffValues.push_back(0);
            }
            else
            {
                cerr << fgred << "  In component: " << Name << " unknown property "
                     << coeff << " referenced. Aborting" << endl;
                exit(-1);
            }
        }
        else
        {
            CoeffNodes.push_back( NULL );
            CoeffValues.push_back(atof(coeff.c_str()));
        }
        coeff_element = element->FindNextElement("coeff");
    }
    coeff_size = CoeffNodes.size();
    inputs = new double[input_size+coeff_size];

    Element *plugin_element = element->FindElement("plugin");
    if (plugin_element)
    {
        string type = plugin_element->GetAttributeValue("type");
        string name = plugin_element->GetAttributeValue("name");
        string func = plugin_element->GetAttributeValue("func");
        if (type == "dll")
        {
            if (g_ltdlCount == 0u)
            {
                lt_dlinit();
            }
            ++g_ltdlCount;

            bool has=false;
            for (PluginListIter i = g_plugin_list.begin(); i != g_plugin_list.end(); ++i)
            {
                if (i->name == name && i->type == Plugin::DLL_FUNC)
                {
                    has = true;
                    is_Ok = true;
                    id = i;
                    break;
                }
            }
            if (!has)
            {
                Plugin tmp;
                tmp.type = Plugin::DLL_FUNC;
                tmp.name = name;
#ifdef _WIN32
                string file_name = name + ".dll";
#else
                string file_name = name + ".so";
#endif
                tmp.FunctionLib = lt_dlopen ((PluginDir + "/" + file_name).c_str());
                const char *dlError = lt_dlerror ();
                if (!dlError)
                {
                    if (func.empty())
                    {
                        func = "_run";
                    }
                    tmp.plugin_main = (PlugInFunc_ptr)lt_dlsym (tmp.FunctionLib, func.c_str());
                    dlError = lt_dlerror ();
                    if (!dlError)
                    {
                        g_plugin_list.push_back(tmp);
                        id = g_plugin_list.end();
                        --id;
                    }
                    else
                    {
                        //cerr << "Fail to find function \"run\" int the plug-in " << PluginDir + '/' + tab.PlugInName << endl;
                        lt_dlclose (tmp.FunctionLib);
                    }
                    {
                        states[STATUS] = 0;
                        is_Ok = true;
                        call(0);
                    }

                }
            }
			else
			{
			  states[STATUS] = 0;
			  call(0);
			}
        }
        else
        {
            use_builtin = true;
        }
    }

    FGFCSComponent::bind();
    Debug(0);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

FGFCSPlugin::~FGFCSPlugin()
{
    call(9);
    if (!use_builtin)
    {
        Plugin & cur = *id;
        switch (cur.type)
        {
        case Plugin::DLL_FUNC :
            call(-1);
            if (--g_ltdlCount == 0u)
            {
                for (PluginListIter i = g_plugin_list.begin(); i != g_plugin_list.end(); ++i)
                {
                    if (i->type == Plugin::DLL_FUNC)
                    {
                        lt_dlclose (i->FunctionLib);
                    }
                }
                g_plugin_list.clear();

                lt_dlexit();
            }
            break;
        default :
            break;
        }
    }
    Debug(1);
	delete []inputs;
	delete []outputs;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCSPlugin::SetOutput(void)
{
    for (unsigned int i=0; i<OutputNodes.size(); i++) OutputNodes[i]->setDoubleValue(outputs[i]);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFCSPlugin::call(int stage )
{
    if (!is_Ok)
        return false;

    for (unsigned i = 0; i<InputNodes.size(); ++i)
    {
        inputs[i] = InputNodes[i]->getDoubleValue() * InputSigns[i];
    }
    for (unsigned i = 0; i<CoeffNodes.size(); ++i)
    {
        if (CoeffNodes[i])
        {
            inputs[i+input_size] = CoeffNodes[i]->getDoubleValue() * CoeffSigns[i];
        }
        else
        {
            inputs[i+input_size] = CoeffValues[i] * CoeffSigns[i];
        }
    }

    int rslt=-1;
    if (use_builtin)
    {
        rslt = plugin_run(stage, input_size+coeff_size, inputs, output_size, outputs, states);
    }
    else
    {
        Plugin & cur = *id;
        switch (cur.type)
        {
        case Plugin::DLL_FUNC :
        {
            rslt = cur.result = (*cur.plugin_main)(stage, input_size+coeff_size, inputs, output_size, outputs, states);
        }
        break;
        default :
            break;
        }
    }

	if (stage == 0)
	{
	
	}
    else if (rslt==0)
    {
        Output = outputs[0];

        Clip();
        if (IsOutput) SetOutput();
    }

    return true;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

bool FGFCSPlugin::Run(void )
{
    if (!is_Ok)
        return false;

    int stage = 1;
    if (fcs->GetTrimStatus())
    {
        stage = 5;
    }

    return call(stage);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCSPlugin::SaveStates(void)
{
    for (int i=0; i<STATE_SIZE; ++i)
    {
        saved_states[i]=states[i];
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCSPlugin::RestoreStates(void)
{
    for (int i=0; i<STATE_SIZE; ++i)
    {
        states[i]=saved_states[i];
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCSPlugin::ResetPastStates(void)
{
    for (int i=STATE_START; i<STATE_START+255; ++i)
    {
        states[i]=0;
    }
    for (int i=STATE_DIV_START; i<STATE_DIV_START+255; ++i)
    {
        states[i]=0;
    }
    for (int i=STATE_ALL_START; i<STATE_ALL_START+500; ++i)
    {
        states[i]=0;
    }
    for (int i=STATE_ALL_DIV_START; i<STATE_ALL_DIV_START+500; ++i)
    {
        states[i]=0;
    }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

unsigned FGFCSPlugin::GetStateNum() const
{
  return (unsigned)(states[STATE_NUM]+0.5);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


double FGFCSPlugin::GetState(unsigned idx) const
{
  return states[idx];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void FGFCSPlugin::SetState(unsigned idx,double val)
{
    states[idx] = val;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

double FGFCSPlugin::GetStateDot(unsigned idx) const
{
  return states[idx+STATE_DIV_START];
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//    The bitmasked value choices are as follows:
//    unset: In this case (the default) JSBSim would only print
//       out the normally expected messages, essentially echoing
//       the config files as they are read. If the environment
//       variable is not set, debug_lvl is set to 1 internally
//    0: This requests JSBSim not to output any messages
//       whatsoever.
//    1: This value explicity requests the normal JSBSim
//       startup messages
//    2: This value asks for a message to be printed out when
//       a class is instantiated
//    4: When this value is set, a message is displayed when a
//       FGModel object executes its Run() method
//    8: When this value is set, various runtime state variables
//       are printed out periodically
//    16: When set various parameters are sanity checked and
//       a message is printed out when they go out of bounds

void FGFCSPlugin::Debug(int from)
{
  if (debug_lvl <= 0) return;

  if (debug_lvl & 1) { // Standard console startup message output
    if (from == 0) { // Constructor
      if (InputNodes.size()>0)
        cout << "      INPUT: " << InputNodes[0]->GetName() << endl;
//    cout << "      Function: " << endl;
      if (IsOutput) {
        for (unsigned int i=0; i<OutputNodes.size(); i++)
          cout << "      OUTPUT: " << OutputNodes[i]->getName() << endl;
      }
    }
  }
  if (debug_lvl & 2 ) { // Instantiation/Destruction notification
    if (from == 0) cout << "Instantiated: FGFCSPlugin" << endl;
    if (from == 1) cout << "Destroyed:    FGFCSPlugin" << endl;
  }
  if (debug_lvl & 4 ) { // Run() method entry print for FGModel-derived objects
  }
  if (debug_lvl & 8 ) { // Runtime state variables
  }
  if (debug_lvl & 16) { // Sanity checking
  }
  if (debug_lvl & 64) {
    if (from == 0) { // Constructor
      cout << IdSrc << endl;
      cout << IdHdr << endl;
    }
  }
}
}

#endif
