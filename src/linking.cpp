/****************************************************************
**linking.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-03.
*
* Description: Tells the linker to include all modules.
*
*****************************************************************/
#include "linking.hpp"

// Revolution Now
#include "colony-mfg.hpp"
#include "player.hpp"
#include "sound.hpp"
#include "turn.hpp"

namespace rn {

void linker_dont_discard_me() {
  linker_dont_discard_module_player();
  linker_dont_discard_module_sound();
  linker_dont_discard_module_colony_mfg();
  // Add more here as needed.
}

} // namespace rn
