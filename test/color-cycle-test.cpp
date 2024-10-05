/****************************************************************
**color-cycle-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-04.
*
* Description: Unit tests for the color-cycle module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/color-cycle.hpp"

// Testing.
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/render/irenderer.hpp"

// Revolution Now
#include "src/co-scheduler.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep
#include "wait.hpp"

namespace rn {
namespace {

using namespace std;
using namespace std::chrono;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[color-cycle] cycle_map_colors_thread" ) {
  rr::MockRenderer renderer;
  MockIGui         gui;

  bool enabled = {};

  auto f = [&] {
    return cycle_map_colors_thread( renderer, gui, enabled );
  };

  wait<> w;

  auto loop = [&]( int const stage, bool const first = false ) {
    renderer.EXPECT__set_color_cycle_stage( stage );
    wait_promise<microseconds> p;
    gui.EXPECT__wait_for( 600ms ).returns( p.wait() );
    if( first )
      w = f();
    else
      run_all_cpp_coroutines();
    REQUIRE( !w.exception() );
    REQUIRE( !w.ready() );
    p.set_value( 600ms );
  };

  // Initialization.
  renderer.EXPECT__get_color_cycle_span().returns( 4 );

  // loop (start in a disabled state).
  enabled = false;
  loop( 0, /*first=*/true );
  loop( 0 );
  loop( 0 );
  loop( 0 );
  loop( 0 );
  loop( 0 );
  loop( 0 );

  enabled = true;
  loop( 0 );
  loop( 1 );
  loop( 2 );
  loop( 3 );
  loop( 0 );
  loop( 1 );
  loop( 2 );
  loop( 3 );

  enabled = false; // Disable at end of cycle.
  loop( 0 );
  loop( 0 );
  loop( 0 );
  loop( 0 );

  enabled = true;
  loop( 0 );
  loop( 1 );
  loop( 2 );
  loop( 3 );
  loop( 0 );
  loop( 1 );
  loop( 2 );
  loop( 3 );
  loop( 0 );

  enabled = false; // Disable in middle of cycle.
  loop( 1 );
  loop( 2 );
  loop( 3 );
  loop( 0 );
  loop( 0 );
  loop( 0 );
  loop( 0 );
  loop( 0 );
  loop( 0 );
  loop( 0 );

  // Should be last; for the scope-exit.
  renderer.EXPECT__set_color_cycle_stage( 0 );
}

} // namespace
} // namespace rn
