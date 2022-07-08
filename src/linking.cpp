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
#include "co-lua.hpp"
#include "conductor.hpp"
#include "config-files.hpp"
#include "error.hpp"
#include "fathers.hpp"
#include "lua-ui.hpp"
#include "map-gen.hpp"
#include "player.hpp"
#include "sound.hpp"
#include "time.hpp"
#include "turn.hpp"

// ss
#include "ss/land-view.hpp"
#include "ss/map-square.hpp"

// base
#include "base/stack-trace.hpp"

namespace rn {

void linker_dont_discard_me() {
  linker_dont_discard_module_player();
  linker_dont_discard_module_sound();
  linker_dont_discard_module_error();
  conductor::linker_dont_discard_module_conductor();
  linker_dont_discard_module_co_lua();
  linker_dont_discard_module_lua_ui();
  linker_dont_discard_module_map_gen();
  linker_dont_discard_module_config_files();
  linker_dont_discard_module_fathers();
  linker_dont_discard_module_time();
  linker_dont_discard_module_gs_land_view();
  linker_dont_discard_module_gs_map_square();
  // Add more here as needed.
}

void dont_optimize_me( void* ) {
  // Do nothing.
}

} // namespace rn
