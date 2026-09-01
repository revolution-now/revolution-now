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

void linker_dont_discard_module_player();
void linker_dont_discard_module_conductor();
void linker_dont_discard_module_co_lua();
void linker_dont_discard_module_lua_ui();
void linker_dont_discard_module_time();
void linker_dont_discard_module_native_expertise();
void linker_dont_discard_module_ss_lua_root_0();
void linker_dont_discard_module_ss_lua_root_1();
void linker_dont_discard_module_ss_lua_root_2();
void linker_dont_discard_module_ss_lua_root_3();
void linker_dont_discard_module_ss_lua_root_4();
void linker_dont_discard_module_ss_lua_root_5();
void linker_dont_discard_module_ss_lua_root_6();
void linker_dont_discard_module_ss_lua_root_7();
void linker_dont_discard_module_ss_lua_root_8();
void linker_dont_discard_module_ss_lua_root_9();

void linker_dont_discard_me() {
  linker_dont_discard_module_player();
  linker_dont_discard_module_conductor();
  linker_dont_discard_module_co_lua();
  linker_dont_discard_module_lua_ui();
  linker_dont_discard_module_time();
  linker_dont_discard_module_native_expertise();
  linker_dont_discard_module_ss_lua_root_0();
  linker_dont_discard_module_ss_lua_root_1();
  linker_dont_discard_module_ss_lua_root_2();
  linker_dont_discard_module_ss_lua_root_3();
  linker_dont_discard_module_ss_lua_root_4();
  linker_dont_discard_module_ss_lua_root_5();
  linker_dont_discard_module_ss_lua_root_6();
  linker_dont_discard_module_ss_lua_root_7();
  linker_dont_discard_module_ss_lua_root_8();
  linker_dont_discard_module_ss_lua_root_9();
  // Add more here as needed.
}

void dont_optimize_me( void* ) {
  // Do nothing.
}

} // namespace rn
