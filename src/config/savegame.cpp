/****************************************************************
**savegame.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-05.
*
* Description: Config info for game saving.
*
*****************************************************************/
#include "savegame.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

}

/****************************************************************
** AutosaveSlot
*****************************************************************/
valid_or<string> config::savegame::AutosaveSlot::validate()
    const {
  REFL_VALIDATE(
      frequency > 0,
      "auto-save slot frequency must be larger than zero." );

  return valid;
}

} // namespace rn
