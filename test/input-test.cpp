/****************************************************************
**input-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-22.
*
* Description: Unit tests for the input module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/input.hpp"

// sdl
#include "src/sdl/include-sdl-base.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn::input {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[input] has_mod_key" ) {
  key_event_t event;

  auto f = [&] { return has_mod_key( event ); };

  REQUIRE_FALSE( f() );

  event.keycode = ::SDLK_LSHIFT;
  REQUIRE_FALSE( f() );

  event.mod.l_alt_down = true;
  REQUIRE( f() );

  event.keycode = ::SDLK_a;
  REQUIRE( f() );

  event.mod.l_alt_down = false;
  REQUIRE_FALSE( f() );

  event.mod.l_shf_down = true;
  REQUIRE( f() );
  event.mod.l_shf_down = false;
  REQUIRE_FALSE( f() );

  event.mod.r_shf_down = true;
  REQUIRE( f() );
  event.mod.r_shf_down = false;
  REQUIRE_FALSE( f() );

  event.mod.shf_down = true;
  REQUIRE( f() );
  event.mod.shf_down = false;
  REQUIRE_FALSE( f() );

  event.mod.l_alt_down = true;
  REQUIRE( f() );
  event.mod.l_alt_down = false;
  REQUIRE_FALSE( f() );

  event.mod.r_alt_down = true;
  REQUIRE( f() );
  event.mod.r_alt_down = false;
  REQUIRE_FALSE( f() );

  event.mod.alt_down = true;
  REQUIRE( f() );
  event.mod.alt_down = false;
  REQUIRE_FALSE( f() );

  event.mod.l_ctrl_down = true;
  REQUIRE( f() );
  event.mod.l_ctrl_down = false;
  REQUIRE_FALSE( f() );

  event.mod.r_ctrl_down = true;
  REQUIRE( f() );
  event.mod.r_ctrl_down = false;
  REQUIRE_FALSE( f() );

  event.mod.ctrl_down = true;
  REQUIRE( f() );
  event.mod.ctrl_down = false;
  REQUIRE_FALSE( f() );
}

} // namespace
} // namespace rn::input
