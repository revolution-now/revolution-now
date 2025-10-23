/****************************************************************
**gs-settings.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-28.
*
* Description: Save-game state for game-wide settings.
*
*****************************************************************/
#include "ss/settings.hpp"

// refl
#include "refl/ext.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

}

/****************************************************************
** SettingsState
*****************************************************************/
valid_or<string> SettingsState::validate() const {
  return valid;
}

} // namespace rn
