/****************************************************************
**menu-plane-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-25.
*
* Description: Unit tests for the menu-plane module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/menu-plane.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/iengine.hpp"
#include "test/mocks/iplane.hpp"
#include "test/mocks/video/ivideo.hpp"

// Revolution Now
#include "src/co-runner.hpp"
#include "src/frame.hpp"
#include "src/input.hpp"
#include "src/mock/matchers.hpp"

// config
#include "src/config/menu-items.rds.hpp"

// gfx
#include "src/gfx/resolution.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;
using namespace input;

using ::gfx::rect;
using ::gfx::size;
using ::mock::matchers::_;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[menu-plane] registration/handlers" ) {
  world w;
  MockIEngine& engine    = w.engine();
  vid::MockIVideo& video = w.video();
  engine.EXPECT__video().returns( video );
  vid::WindowHandle const wh;
  engine.EXPECT__window().returns( wh );
  gfx::Resolutions resolutions{
    .scored   = { gfx::ScoredResolution{
        .resolution =
          gfx::Resolution{
              .physical_window = { .w = 1280, .h = 720 },
              .logical         = { .w = 640, .h = 360 },
              .scale           = 2 } } },
    .selected = 0 };
  engine.EXPECT__resolutions().returns( resolutions );
  MenuPlane mp( engine );
  MockIPlane mock_plane;

  REQUIRE_FALSE(
      mp.can_handle_menu_click( e_menu_item::zoom_in ) );
  REQUIRE_FALSE(
      mp.can_handle_menu_click( e_menu_item::zoom_out ) );

  {
    auto const deregistrar =
        mp.register_handler( e_menu_item::zoom_in, mock_plane );
    mock_plane
        .EXPECT__will_handle_menu_click( e_menu_item::zoom_in )
        .returns( false );
    REQUIRE_FALSE(
        mp.can_handle_menu_click( e_menu_item::zoom_in ) );
    REQUIRE_FALSE(
        mp.can_handle_menu_click( e_menu_item::zoom_out ) );

    mock_plane
        .EXPECT__will_handle_menu_click( e_menu_item::zoom_in )
        .returns( true );
    REQUIRE( mp.can_handle_menu_click( e_menu_item::zoom_in ) );
    REQUIRE_FALSE(
        mp.can_handle_menu_click( e_menu_item::zoom_out ) );
  }

  REQUIRE_FALSE(
      mp.can_handle_menu_click( e_menu_item::zoom_in ) );
  REQUIRE_FALSE(
      mp.can_handle_menu_click( e_menu_item::zoom_out ) );

  auto const deregistrar =
      mp.register_handler( e_menu_item::zoom_in, mock_plane );

  int clicked_zoom_in = 0;
  mock_plane
      .EXPECT__will_handle_menu_click( e_menu_item::zoom_in )
      .returns( true );
  mock_plane.EXPECT__handle_menu_click( e_menu_item::zoom_in )
      .invokes( [&] { ++clicked_zoom_in; } );
  REQUIRE_FALSE( mp.click_item( e_menu_item::zoom_out ) );
  REQUIRE( clicked_zoom_in == 0 );
  REQUIRE( mp.click_item( e_menu_item::zoom_in ) );
  REQUIRE( clicked_zoom_in == 1 );
}

TEST_CASE( "[menu-plane] open_menu" ) {
  world W;
  MockIEngine& engine    = W.engine();
  vid::MockIVideo& video = W.video();
  engine.EXPECT__video().by_default().returns( video );
  vid::WindowHandle const wh;
  engine.EXPECT__window().by_default().returns( wh );
  gfx::Resolutions resolutions{
    .scored   = { gfx::ScoredResolution{
        .resolution =
          gfx::Resolution{
              .physical_window = { .w = 1280, .h = 720 },
              .logical         = { .w = 640, .h = 360 },
              .scale           = 2 } } },
    .selected = 0 };
  engine.EXPECT__resolutions().by_default().returns(
      resolutions );
  MenuPlane mp( engine );
  MockIPlane mock_plane;
  IPlane& plane_impl = mp.impl();

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

  wait<maybe<e_menu_item>> const w =
      mp.open_menu( e_menu::view, positions );
  REQUIRE( !w.ready() );

  auto const send_key = [&]( ::SDL_Keycode const key ) {
    key_event_t e;
    e.keycode      = key;
    e.change       = e_key_change::down;
    auto const res = plane_impl.input( e );
    run_all_coroutines();
    return res;
  };

  auto const finish_click_animation = [&] {
    for( int i = 0; i < 24; ++i ) {
      testing_notify_all_subscribers();
      run_all_coroutines();
    }
  };

  auto const deregistrar =
      mp.register_handler( e_menu_item::zoom_in, mock_plane );

  SECTION( "select item" ) {
    mock_plane
        .EXPECT__will_handle_menu_click( e_menu_item::zoom_in )
        .returns( true );
    REQUIRE( send_key( ::SDLK_DOWN ) == e_input_handled::yes );
    REQUIRE( send_key( ::SDLK_RETURN ) == e_input_handled::yes );
    finish_click_animation();
    REQUIRE( w.ready() );
    REQUIRE( w->has_value() );
    REQUIRE( **w == e_menu_item::zoom_in );
  }

  SECTION( "close_all_menus" ) {
    mp.close_all_menus();
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( !w->has_value() );
  }

  SECTION( "escape" ) {
    send_key( ::SDLK_ESCAPE );
    mp.close_all_menus();
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( !w->has_value() );
  }
}

} // namespace
} // namespace rn
