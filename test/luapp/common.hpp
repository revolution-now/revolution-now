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

// base
#include "base/error.hpp"

namespace lua {

struct harness {
  state   st;
  cthread L = st.thread.main().cthread();
  c_api   C = L;

  ~harness() {
    if( st.alive() ) { CHECK_EQ( C.stack_size(), 0 ); }
  }
};

} // namespace lua

#define LUA_TEST_CASE( ... ) \
  TEST_CASE_METHOD( ::lua::harness, __VA_ARGS__ )
