/****************************************************************
**menu-body-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-25.
*
* Description: Unit tests for the menu-body module.
*
*****************************************************************/
#include "frame.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/menu-body.hpp"

// Testing.
#include "test/mocking.hpp"
#include "test/mocks/imenu-server.hpp"
#include "test/mocks/render/itextometer.hpp"

// Revolution Now
#include "src/co-runner.hpp"
#include "src/frame.hpp"
#include "src/input.hpp"
#include "src/menu-render.hpp"
#include "src/mock/matchers.hpp"

// config
#include "src/config/menu-items.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;
using namespace input;

using ::Catch::Matchers::WithinAbs;
using ::gfx::rect;
using ::gfx::size;
using ::mock::matchers::_;

/****************************************************************
** Harness
*****************************************************************/
struct Harness {
  [[maybe_unused]] Harness()
    : threads_( menu_server_, textometer_ ) {}

  void send_key( ::SDL_Keycode const key ) {
    key_event_t e;
    e.keycode = key;
    e.change  = e_key_change::down;
    threads_.send_event( MenuEventRaw::device{ .event = e } );
  }

  vector<int> open_menu_ids() const {
    vector<int> res;
    for( int const menu_id : threads_.open_menu_ids() )
      res.push_back( menu_id );
    return res;
  }

  void expect_click_animation_and_fade(
      wait<maybe<e_menu_item>> const& w, string const& menu_name,
      string const& item_name ) {
    // Animation: blink off
    REQUIRE( threads_.anim_state( 1 ).highlighted == menu_name );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );
    REQUIRE( threads_.anim_state( 2 ).highlighted == nothing );
    REQUIRE( threads_.anim_state( 2 ).alpha == 1.0 );
    // Animation: blink off
    testing_notify_all_subscribers();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 2 );
    REQUIRE( open_menu_ids() == vector<int>{ 1, 2 } );
    // Animation: blink on
    REQUIRE( threads_.anim_state( 1 ).highlighted == menu_name );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );
    REQUIRE( threads_.anim_state( 2 ).highlighted == item_name );
    REQUIRE( threads_.anim_state( 2 ).alpha == 1.0 );
    // Animation: fade out, first frame.
    testing_notify_all_subscribers();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 2 );
    REQUIRE( open_menu_ids() == vector<int>{ 1, 2 } );
    // Animation: blink on
    REQUIRE( threads_.anim_state( 1 ).highlighted == menu_name );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );
    REQUIRE( threads_.anim_state( 2 ).highlighted == item_name );
    REQUIRE( threads_.anim_state( 2 ).alpha ==
             Approx( .954545 ) );
    // Animation: fade out, remainder but for last.
    for( int i = 0; i < 21; ++i ) {
      testing_notify_all_subscribers();
      run_all_coroutines();
    }
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 2 );
    REQUIRE( open_menu_ids() == vector<int>{ 1, 2 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted == menu_name );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );
    REQUIRE( threads_.anim_state( 2 ).highlighted == item_name );
    REQUIRE_THAT( threads_.anim_state( 2 ).alpha,
                  WithinAbs( 0.0, 0.000001 ) );
    // Animation: fade out, last.
    testing_notify_all_subscribers();
    run_all_coroutines();
  }

  void expect_menu_bar_dimensions() {
    textometer_.EXPECT__font_height().by_default().returns( 8 );
    string const headers[] = { "" };
    textometer_
        .EXPECT__dimensions_for_line( rr::TextLayout{}, _ )
        .by_default()
        .returns( size{ .w = 6, .h = 8 } );
  }

  inline static rect const kScreen{
    .origin = {}, .size = { .w = 640, .h = 360 } };

  MockIMenuServer menu_server_;
  rr::MockTextometer textometer_;
  MenuThreads threads_;
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE_METHOD( Harness, "[menu-body] open_menu" ) {
  e_menu menu = {};
  MenuAllowedPositions const positions{
    .positions_allowed = {
      MenuAllowedPosition{
        .where       = { .x = 32, .y = 16 },
        .orientation = e_diagonal_direction::ne,
        .parent_side = {},
      },
      MenuAllowedPosition{
        .where       = { .x = 32, .y = 16 },
        .orientation = e_diagonal_direction::nw,
        .parent_side = {},
      },
    } };

  auto f = [&] {
    return threads_.open_menu( menu, kScreen, positions );
  };

  menu = e_menu::view;

  SECTION( "default" ) {
    REQUIRE( threads_.open_count() == 0 );
    REQUIRE( open_menu_ids() == vector<int>{} );
  }

  SECTION(
      "open Window submenu and escape twice to close both" ) {
    expect_menu_bar_dimensions();
    wait<maybe<e_menu_item>> const w = f();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 1 );
    REQUIRE( open_menu_ids() == vector<int>{ 1 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted == nothing );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );

    send_key( ::SDLK_UP );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 1 );
    REQUIRE( open_menu_ids() == vector<int>{ 1 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted ==
             "Window  " );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );

    send_key( ::SDLK_RETURN );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 2 );
    REQUIRE( open_menu_ids() == vector<int>{ 1, 2 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted ==
             "Window  " );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );
    REQUIRE( threads_.anim_state( 2 ).highlighted == nothing );
    REQUIRE( threads_.anim_state( 2 ).alpha == 1.0 );

    send_key( ::SDLK_DOWN );
    menu_server_
        .EXPECT__can_handle_menu_click(
            e_menu_item::toggle_fullscreen )
        .returns( true );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 2 );
    REQUIRE( open_menu_ids() == vector<int>{ 1, 2 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted ==
             "Window  " );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );
    REQUIRE( threads_.anim_state( 2 ).highlighted ==
             "Toggle Fullscreen" );
    REQUIRE( threads_.anim_state( 2 ).alpha == 1.0 );

    send_key( ::SDLK_ESCAPE );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 1 );
    REQUIRE( open_menu_ids() == vector<int>{ 1 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted ==
             "Window  " );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );

    send_key( ::SDLK_ESCAPE );
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( threads_.open_count() == 0 );
    REQUIRE( open_menu_ids() == vector<int>{} );
  }

  SECTION(
      "open Window submenu and send close_all to close both" ) {
    expect_menu_bar_dimensions();
    wait<maybe<e_menu_item>> const w = f();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 1 );
    REQUIRE( open_menu_ids() == vector<int>{ 1 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted == nothing );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );

    send_key( ::SDLK_UP );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 1 );
    REQUIRE( open_menu_ids() == vector<int>{ 1 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted ==
             "Window  " );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );

    send_key( ::SDLK_RETURN );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 2 );
    REQUIRE( open_menu_ids() == vector<int>{ 1, 2 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted ==
             "Window  " );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );
    REQUIRE( threads_.anim_state( 2 ).highlighted == nothing );
    REQUIRE( threads_.anim_state( 2 ).alpha == 1.0 );

    send_key( ::SDLK_DOWN );
    menu_server_
        .EXPECT__can_handle_menu_click(
            e_menu_item::toggle_fullscreen )
        .returns( true );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 2 );
    REQUIRE( open_menu_ids() == vector<int>{ 1, 2 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted ==
             "Window  " );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );
    REQUIRE( threads_.anim_state( 2 ).highlighted ==
             "Toggle Fullscreen" );
    REQUIRE( threads_.anim_state( 2 ).alpha == 1.0 );

    threads_.send_event( MenuEventRaw::close_all{} );
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( threads_.open_count() == 0 );
    REQUIRE( open_menu_ids() == vector<int>{} );
  }

  SECTION( "open Window submenu and select toggle fullscreen" ) {
    expect_menu_bar_dimensions();
    wait<maybe<e_menu_item>> const w = f();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 1 );
    REQUIRE( open_menu_ids() == vector<int>{ 1 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted == nothing );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );

    send_key( ::SDLK_UP );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 1 );
    REQUIRE( open_menu_ids() == vector<int>{ 1 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted ==
             "Window  " );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );

    send_key( ::SDLK_RETURN );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 2 );
    REQUIRE( open_menu_ids() == vector<int>{ 1, 2 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted ==
             "Window  " );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );
    REQUIRE( threads_.anim_state( 2 ).highlighted == nothing );
    REQUIRE( threads_.anim_state( 2 ).alpha == 1.0 );

    send_key( ::SDLK_DOWN );
    menu_server_
        .EXPECT__can_handle_menu_click(
            e_menu_item::toggle_fullscreen )
        .returns( true );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 2 );
    REQUIRE( open_menu_ids() == vector<int>{ 1, 2 } );
    REQUIRE( threads_.anim_state( 1 ).highlighted ==
             "Window  " );
    REQUIRE( threads_.anim_state( 1 ).alpha == 1.0 );
    REQUIRE( threads_.anim_state( 2 ).highlighted ==
             "Toggle Fullscreen" );
    REQUIRE( threads_.anim_state( 2 ).alpha == 1.0 );

    send_key( ::SDLK_RETURN );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( threads_.open_count() == 2 );
    REQUIRE( open_menu_ids() == vector<int>{ 1, 2 } );
    expect_click_animation_and_fade( w, "Window  ",
                                     "Toggle Fullscreen" );
    REQUIRE( w.ready() );
    REQUIRE( w->has_value() );
    REQUIRE( **w == e_menu_item::toggle_fullscreen );
    REQUIRE( threads_.open_count() == 0 );
    REQUIRE( open_menu_ids() == vector<int>{} );
  }
}

} // namespace
} // namespace rn
