/****************************************************************
**menu-bar-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-25.
*
* Description: Unit tests for the menu-bar module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/menu-bar.hpp"

// Testing.
#include "test/mocks/imenu-server.hpp"

// Revolution Now
#include "src/co-runner.hpp"
#include "src/input.hpp"
#include "src/menu-render.hpp"
#include "src/mock/matchers.hpp"

// config
#include "src/config/menu-items.rds.hpp"
#include "src/config/menu.rds.hpp"

// refl
#include "src/refl/query-enum.hpp"
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;
using namespace input;

using ::gfx::rect;
using ::mock::matchers::_;
using ::refl::enum_values;

/****************************************************************
** Harness
*****************************************************************/
struct Harness {
  [[maybe_unused]] Harness() : bar_( menu_server_ ) {}

  [[nodiscard]] bool send_key( ::SDL_Keycode const key ) {
    key_event_t e;
    e.keycode = key;
    e.change  = e_key_change::down;
    return bar_.send_event(
        MenuBarEventRaw::device{ .event = e } );
  }

  [[nodiscard]] bool send_alt_key( ::SDL_Keycode const key ) {
    key_event_t e;
    e.mod.alt_down   = true;
    e.mod.l_alt_down = true;
    e.keycode        = key;
    e.change         = e_key_change::down;
    return bar_.send_event(
        MenuBarEventRaw::device{ .event = e } );
  }

  inline static rect const kScreen{
    .origin = {}, .size = { .w = 640, .h = 360 } };

  MockIMenuServer menu_server_;
  MenuBar bar_;
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE_METHOD( Harness, "[menu-bar] run_thread" ) {
  vector<e_menu> const& contents = config_menu.menu_bar;

  auto f = [&] { return bar_.run_thread( kScreen, contents ); };

  SECTION( "no input" ) {
    wait<> const w = f();
    REQUIRE( !w.ready() );
    REQUIRE( bar_.anim_state().opened_menu() == nothing );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
  }

  SECTION( "Alt highlights first menu (game)" ) {
    wait<> const w = f();
    REQUIRE( !w.ready() );
    REQUIRE( send_key( ::SDLK_LALT ) );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == nothing );
    REQUIRE( bar_.anim_state().highlighted_menu() ==
             e_menu::game );
  }

  SECTION(
      "Alt-G opens game menu, then close w/ no selection" ) {
    wait<> const w = f();
    REQUIRE( !w.ready() );
    REQUIRE( send_alt_key( ::SDLK_g ) );
    menu_server_.EXPECT__close_all_menus();
    wait_promise<maybe<e_menu_item>> p;
    menu_server_.EXPECT__open_menu( e_menu::game, _ )
        .returns( p.wait() );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == e_menu::game );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
    p.set_value( nothing );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == nothing );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
  }

  SECTION( "Alt-G, then select an item, no handler" ) {
    wait<> const w = f();
    REQUIRE( !w.ready() );
    REQUIRE( send_alt_key( ::SDLK_g ) );
    menu_server_.EXPECT__close_all_menus();
    wait_promise<maybe<e_menu_item>> p;
    menu_server_.EXPECT__open_menu( e_menu::game, _ )
        .returns( p.wait() );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == e_menu::game );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
    p.set_value( e_menu_item::exit );
    menu_server_
        .EXPECT__can_handle_menu_click( e_menu_item::exit )
        .returns( false );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == nothing );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
  }

  SECTION( "Alt-G, then select an item, with handler" ) {
    wait<> const w = f();
    REQUIRE( !w.ready() );
    REQUIRE( send_alt_key( ::SDLK_g ) );
    menu_server_.EXPECT__close_all_menus();
    wait_promise<maybe<e_menu_item>> p;
    menu_server_.EXPECT__open_menu( e_menu::game, _ )
        .returns( p.wait() );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == e_menu::game );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
    p.set_value( e_menu_item::exit );
    menu_server_
        .EXPECT__can_handle_menu_click( e_menu_item::exit )
        .returns( true );
    menu_server_.EXPECT__click_item( e_menu_item::exit )
        .returns( true );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == nothing );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
  }

  SECTION( "Alt-G, then right to open view menu" ) {
    wait<> const w = f();
    REQUIRE( !w.ready() );

    REQUIRE( send_alt_key( ::SDLK_g ) );
    menu_server_.EXPECT__close_all_menus();
    wait_promise<maybe<e_menu_item>> p_game;
    menu_server_.EXPECT__open_menu( e_menu::game, _ )
        .returns( p_game.wait() );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == e_menu::game );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );

    REQUIRE( send_key( ::SDLK_RIGHT ) );
    menu_server_.EXPECT__close_all_menus();
    wait_promise<maybe<e_menu_item>> p_view;
    menu_server_.EXPECT__open_menu( e_menu::view, _ )
        .returns( p_view.wait() );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == e_menu::view );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
  }

  SECTION( "Alt-G, then left to open revolopedia menu" ) {
    wait<> const w = f();
    REQUIRE( !w.ready() );

    REQUIRE( send_alt_key( ::SDLK_g ) );
    menu_server_.EXPECT__close_all_menus();
    wait_promise<maybe<e_menu_item>> p_game;
    menu_server_.EXPECT__open_menu( e_menu::game, _ )
        .returns( p_game.wait() );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == e_menu::game );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );

    REQUIRE( send_key( ::SDLK_LEFT ) );
    menu_server_.EXPECT__close_all_menus();
    wait_promise<maybe<e_menu_item>> p_view;
    menu_server_.EXPECT__open_menu( e_menu::pedia, _ )
        .returns( p_view.wait() );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == e_menu::pedia );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
  }

  // Verifies that escape is not handled by the bar; it should
  // instead be handled by the menu bodies.
  SECTION( "Alt-G opens game menu, escape not handled" ) {
    wait<> const w = f();
    REQUIRE( !w.ready() );
    REQUIRE( send_alt_key( ::SDLK_g ) );
    menu_server_.EXPECT__close_all_menus();
    wait_promise<maybe<e_menu_item>> p;
    menu_server_.EXPECT__open_menu( e_menu::game, _ )
        .returns( p.wait() );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == e_menu::game );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
    REQUIRE_FALSE( send_key( ::SDLK_ESCAPE ) );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == e_menu::game );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
  }

  SECTION( "Alt-G opens game menu, alt closes" ) {
    wait<> const w = f();
    REQUIRE( !w.ready() );
    REQUIRE( send_alt_key( ::SDLK_g ) );
    menu_server_.EXPECT__close_all_menus();
    wait_promise<maybe<e_menu_item>> p;
    menu_server_.EXPECT__open_menu( e_menu::game, _ )
        .returns( p.wait() );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == e_menu::game );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
    REQUIRE( send_key( ::SDLK_LALT ) );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == nothing );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
  }

  SECTION( "Alt-G opens game menu, close event closes" ) {
    wait<> const w = f();
    REQUIRE( !w.ready() );
    REQUIRE( send_alt_key( ::SDLK_g ) );
    menu_server_.EXPECT__close_all_menus();
    wait_promise<maybe<e_menu_item>> p;
    menu_server_.EXPECT__open_menu( e_menu::game, _ )
        .returns( p.wait() );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == e_menu::game );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
    REQUIRE( bar_.send_event( MenuBarEventRaw::close{} ) );
    run_all_coroutines();
    REQUIRE( bar_.anim_state().opened_menu() == nothing );
    REQUIRE( bar_.anim_state().highlighted_menu() == nothing );
  }
}

} // namespace
} // namespace rn
