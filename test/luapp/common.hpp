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
    // As a sanity check, we want to check that when each test
    // case finishes, the Lua stack has zero size, meaning that
    // we've accounted for everything that has been pushed. How-
    // ever, when a test fails for other reasons (e.g. failed RE-
    // QUIRE) then Catch2 will throw an exception which will pre-
    // vent us from cleaning up the stack, and so we only check
    // that the stack size is zero when the test is otherwise
    // successful.
    bool test_case_failed = ( std::uncaught_exceptions() > 0 );

    // Also, some test cases need to close the Lua state before
    // the end of the test in order to test garbage-collection
    // related things. So if that has happened then it is not
    // safe to check the stack size, so check for that as well.
    bool state_closed_early = !st.has_value();

    if( !test_case_failed && !state_closed_early ) {
      CHECK_EQ( C.stack_size(), 0 );
    }
  }
};

} // namespace lua

#define LUA_TEST_CASE( ... ) \
  TEST_CASE_METHOD( ::lua::harness, __VA_ARGS__ )
