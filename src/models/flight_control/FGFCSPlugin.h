/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 Header:       FGFCSPlugin.h
 Author:       Matthew Gong
 Date started: 2008

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

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGFCSPLUGIN_H
#define FGFCSPLUGIN_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "FGFCSComponent.h"
#include <input_output/FGXMLElement.h>

#include <vector>
using std::vector;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_FCSPLUGIN "$Id: FGFCSPlugin.h $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGFCS;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** Models a FCSPlugin object.
    @author Matthew Gong

One of the most recent additions to the FCS component set is the FCS Plugin
component. This component allows a function to be created when no other component
is suitable. The function component is defined as follows:

@code
<fcs_plugin name="Windup Trigger">
  [<input> [-]property </input>]
  [<input> [-]property </input>]
  [<coeff> -fcs/abc </coeff>]
  [<coeff> 12.3 </coeff>]
  <plugin type="{dll | python}" name="?" [func="?"]/>
  [<output> {property} </output>]
  [<output> {property} </output>]
  [<output> {property} </output>]
</ fcs_plugin >
@endcode

    @version $Id: FGFCSPlugin.h $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

struct Plugin;
typedef vector<Plugin> PluginList;
typedef PluginList::iterator PluginListIter;
typedef PluginList::const_iterator PluginListCIter;

class FGFCSPlugin  : public FGFCSComponent
{
public:
  FGFCSPlugin(FGFCS* fcs, Element* element);
  ~FGFCSPlugin();

  bool Run(void);
  void SetOutput(void);
  void ResetPastStates(void);

  /*
   * 998 status
   * 999 state num
   * 1000 dt
   */
  enum{
    STATE_START=0,
    STATE_DIV_START=255,
    STATE_ALL_START=1001,
    STATE_ALL_DIV_START=1501,
    STATE_IDX_START=900,
    STATUS=998,
    STATE_ALL_NUM=997,
    STATE_NUM=999,
	STATE_SIZE=2001,
    DT_VAL=1000,
  };

  void SaveStates(void);
  void RestoreStates(void);
  double GetStateDot(unsigned int i) const;
  void SetState(unsigned int i, double val);
  double GetState(unsigned int i) const;
  unsigned int GetStateNum(void) const;

private:
  PluginListIter id;
  bool     is_Ok;
  double states[STATE_SIZE];
  double saved_states[STATE_SIZE];
  double *inputs, *outputs;
  int output_size, input_size, coeff_size;

  vector <double> CoeffValues;
  vector <FGPropertyManager*> CoeffNodes;
  vector <float> CoeffSigns;

  void Debug(int from);

  bool call(int stage);
  bool use_builtin;
};

}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#endif
