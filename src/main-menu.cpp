/****************************************************************
**main-menu.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Main application menu.
*
*****************************************************************/
#include "main-menu.hpp"

// Rds
#include "main-menu-impl.rds.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "compositor.hpp"
#include "game.hpp"
#include "igui.hpp"
#include "interrupts.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "tiles.hpp"

// config
#include "config/tile-enum.rds.hpp"

// render
#include "render/renderer.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** MainMenuPlane
*****************************************************************/
struct MainMenuPlane : public IPlane {
  // State
  Planes&          planes_;
  IGui&            gui_;
  e_main_menu_item curr_item_ = {};

 public:
  MainMenuPlane( Planes& planes, IGui& gui )
    : planes_( planes ), gui_( gui ) {}

  void draw( rr::Renderer& renderer ) const override {
    UNWRAP_CHECK(
        normal_area,
        compositor::section( compositor::e_section::normal ) );
    {
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .7 );
      rr::Painter painter = renderer.painter();
      tile_sprite( painter, e_tile::wood_middle, normal_area );
    }
  }

  wait<> item_selected( e_main_menu_item item ) {
    switch( item ) {
      case e_main_menu_item::new_random:
        co_await run_game_with_mode( planes_, gui_,
                                     StartMode::new_random{} );
        break;
      case e_main_menu_item::new_america:
        co_await run_game_with_mode( planes_, gui_,
                                     StartMode::new_america{} );
        break;
      case e_main_menu_item::new_customize:
        co_await run_game_with_mode(
            planes_, gui_, StartMode::new_customize{} );
        break;
      case e_main_menu_item::load:
        co_await run_game_with_mode( planes_, gui_,
                                     StartMode::load{} );
        break;
      case e_main_menu_item::hall_of_fame:
        break;
    }
  }

  wait<maybe<e_main_menu_item>> show_menu() {
    EnumChoiceConfig const config{
        .msg = "[REVOLUTION|NOW] Version 1.0 -- 4-Apr-24",
        .cancel_actions = { .disallow_clicking_outside = true,
                            .disallow_escape_key       = false },
    };
    refl::enum_map<e_main_menu_item, std::string> names;
    refl::enum_map<e_main_menu_item, bool>        disabled;

    using enum e_main_menu_item;
    names[new_random]       = "Start a Game in NEW WORLD";
    names[new_america]      = "Start a Game in AMERICA";
    names[new_customize]    = "CUSTOMIZE New World";
    names[load]             = "LOAD Game";
    names[hall_of_fame]     = "View Hall of Fame";
    disabled[new_random]    = false;
    disabled[new_america]   = true;
    disabled[new_customize] = true;
    disabled[load]          = false;
    disabled[hall_of_fame]  = true;

    auto const item = co_await gui_.optional_enum_choice(
        config, names, disabled );
    co_return item;
  }

  wait<> run() {
    while( auto const item = co_await show_menu() )
      co_await item_selected( *item );
  }
};

} // namespace

/****************************************************************
** API
*****************************************************************/
wait<> run_main_menu( Planes& planes, IGui& gui ) {
  auto        owner = planes.push();
  PlaneGroup& group = owner.group;

  MainMenuPlane main_menu_plane( planes, gui );
  group.bottom = &main_menu_plane;

  co_await co::while_throws<main_menu_interrupt>(
      [&] { return main_menu_plane.run(); } );
}

} // namespace rn
