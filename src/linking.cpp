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

namespace rn {

void linker_dont_discard_module_map_updater_lua();
void linker_dont_discard_module_player();
void linker_dont_discard_module_sound();
void linker_dont_discard_module_error();
void linker_dont_discard_module_conductor();
void linker_dont_discard_module_co_lua();
void linker_dont_discard_module_lua_ui();
void linker_dont_discard_module_map_gen();
void linker_dont_discard_module_config_files();
void linker_dont_discard_module_ss_fathers();
void linker_dont_discard_module_time();
void linker_dont_discard_module_ss_land_view();
void linker_dont_discard_module_ss_map_square();
void linker_dont_discard_module_irand();
void linker_dont_discard_module_ss_dwelling();
void linker_dont_discard_module_native_expertise();
void linker_dont_discard_module_ss_native_unit();

void linker_dont_discard_me() {
  linker_dont_discard_module_map_updater_lua();
  linker_dont_discard_module_player();
  linker_dont_discard_module_sound();
  linker_dont_discard_module_error();
  linker_dont_discard_module_conductor();
  linker_dont_discard_module_co_lua();
  linker_dont_discard_module_lua_ui();
  linker_dont_discard_module_map_gen();
  linker_dont_discard_module_config_files();
  linker_dont_discard_module_ss_fathers();
  linker_dont_discard_module_time();
  linker_dont_discard_module_ss_land_view();
  linker_dont_discard_module_ss_map_square();
  linker_dont_discard_module_irand();
  linker_dont_discard_module_ss_dwelling();
  linker_dont_discard_module_native_expertise();
  linker_dont_discard_module_ss_native_unit();
  // Add more here as needed.
}

void dont_optimize_me( void* ) {
  // Do nothing.
}

} // namespace rn
