/****************************************************************
**common.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: Common definitions for luapp unit tests.
*
*****************************************************************/
#pragma once

// luapp
#include "src/luapp/c-api.hpp"
#include "src/luapp/state.hpp"

namespace lua {

struct harness {
  state   global_state;
  c_api   C = global_state.main_cthread();
  cthread L = global_state.main_cthread();
};

} // namespace lua

#define LUA_TEST_CASE( ... ) \
  TEST_CASE_METHOD( ::lua::harness, __VA_ARGS__ )
