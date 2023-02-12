/****************************************************************
**window.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-11.
*
* Description: Unit tests for the src/window.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/window.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[window] num_windows_created / "
    "num_windows_currently_open" ) {
  // FIXME: can't run until we mock the compositor (or screen?).
#if 0
  WindowPlane wp;

  REQUIRE( wp.num_windows_created() == 0 );
  REQUIRE( wp.num_windows_currently_open() == 0 );

  wait<> message_box = wp.message_box( "hello" );
  REQUIRE( wp.num_windows_created() == 1 );
  REQUIRE( wp.num_windows_currently_open() == 1 );

  message_box.cancel();
  REQUIRE( wp.num_windows_created() == 1 );
  REQUIRE( wp.num_windows_currently_open() == 0 );

  message_box         = wp.message_box( "hello" );
  wait<> message_box2 = wp.message_box( "hello2" );
  REQUIRE( wp.num_windows_created() == 3 );
  REQUIRE( wp.num_windows_currently_open() == 2 );

  message_box2.cancel();
  REQUIRE( wp.num_windows_created() == 3 );
  REQUIRE( wp.num_windows_currently_open() == 1 );

  message_box.cancel();
  REQUIRE( wp.num_windows_created() == 3 );
  REQUIRE( wp.num_windows_currently_open() == 0 );
#endif
}

} // namespace
} // namespace rn
