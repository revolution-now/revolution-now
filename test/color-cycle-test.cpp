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
#include "matchers.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/color-cycle.hpp"

// Testing.
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/render/irenderer.hpp"

// Revolution Now
#include "src/co-scheduler.hpp"

// config
#include "src/config/gfx.rds.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep
#include "wait.hpp"

namespace rn {
namespace {

using namespace std;
using namespace std::chrono;

using ::mock::matchers::HasSize;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[color-cycle] cycle_map_colors_thread" ) {
  rr::MockRenderer renderer;
  MockIGui gui;

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

  // loop (start in a disabled state).
  enabled = false;
  loop( 0, /*first=*/true );
  loop( 0 ), loop( 0 ), loop( 0 ), loop( 0 ), loop( 0 );
  loop( 0 ), loop( 0 ), loop( 0 ), loop( 0 ), loop( 0 );
  loop( 0 ), loop( 0 );

  enabled = true;
  loop( 0 ), loop( 1 ), loop( 2 ), loop( 3 ), loop( 4 );
  loop( 5 ), loop( 6 ), loop( 7 ), loop( 8 ), loop( 9 );
  loop( 0 ), loop( 1 ), loop( 2 ), loop( 3 ), loop( 4 );
  loop( 5 ), loop( 6 ), loop( 7 ), loop( 8 ), loop( 9 );

  enabled = false; // Disable at end of cycle.
  loop( 0 ), loop( 0 ), loop( 0 ), loop( 0 ), loop( 0 );
  loop( 0 ), loop( 0 ), loop( 0 ), loop( 0 ), loop( 0 );
  loop( 0 );

  enabled = true;
  loop( 0 ), loop( 1 ), loop( 2 ), loop( 3 ), loop( 4 );
  loop( 5 ), loop( 6 ), loop( 7 ), loop( 8 ), loop( 9 );
  loop( 0 ), loop( 1 ), loop( 2 ), loop( 3 ), loop( 4 );
  loop( 5 ), loop( 6 ), loop( 7 ), loop( 8 ), loop( 9 );
  loop( 0 );

  enabled = false; // Disable in middle of cycle.
  loop( 1 ), loop( 2 ), loop( 3 ), loop( 4 ), loop( 5 );
  loop( 6 ), loop( 7 ), loop( 8 ), loop( 9 ), loop( 0 );
  loop( 0 ), loop( 0 ), loop( 0 ), loop( 0 ), loop( 0 );
  loop( 0 ), loop( 0 ), loop( 0 ), loop( 0 ), loop( 0 );
  loop( 0 ), loop( 0 ), loop( 0 );

  // Should be last; for the scope-exit.
  renderer.EXPECT__set_color_cycle_stage( 0 );
}

TEST_CASE( "[color-cycle] cycle_plan_idx" ) {
  using enum e_color_cycle_plan;

  auto f = [&]( e_color_cycle_plan const plan ) {
    return cycle_plan_idx( plan );
  };

  REQUIRE( f( surf ) == 0 );
  REQUIRE( f( sea_lane ) == 1 );
  REQUIRE( f( river ) == 2 );
}

TEST_CASE( "[color-cycle] set_color_cycle_plans" ) {
  rr::MockRenderer renderer;
  renderer.EXPECT__set_color_cycle_plans( HasSize( 30 ) );
  set_color_cycle_plans( renderer );
}

TEST_CASE( "[color-cycle] set_color_cycle_keys" ) {
  rr::MockRenderer renderer;
  renderer.EXPECT__set_color_cycle_keys( HasSize( 10 ) );
  set_color_cycle_keys( renderer );
}

} // namespace
} // namespace rn
