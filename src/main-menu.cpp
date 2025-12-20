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
#include "game.hpp"
#include "iengine.hpp"
#include "igui.hpp"
#include "interrupts.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "screen.hpp"
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
  IEngine& engine_;
  Planes& planes_;
  IGui& gui_;
  e_main_menu_item curr_item_ = {};

 public:
  MainMenuPlane( IEngine& engine, Planes& planes, IGui& gui )
    : engine_( engine ), planes_( planes ), gui_( gui ) {}

  void draw( rr::Renderer& renderer, Coord ) const override {
    auto const area = main_window_logical_rect(
        engine_.video(), engine_.window(),
        engine_.resolutions() );
    {
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .7 );
      tile_sprite( renderer, e_tile::wood_middle, area );
    }
  }

  void on_logical_resolution_selected(
      gfx::e_resolution ) override {}

  wait<> item_selected( e_main_menu_item item ) {
    switch( item ) {
      case e_main_menu_item::new_random:
        co_await run_game_with_mode( engine_, planes_, gui_,
                                     StartMode::new_random{} );
        break;
      case e_main_menu_item::new_america:
        co_await run_game_with_mode( engine_, planes_, gui_,
                                     StartMode::new_america{} );
        break;
      case e_main_menu_item::new_customize:
        co_await run_game_with_mode(
            engine_, planes_, gui_, StartMode::new_customize{} );
        break;
      case e_main_menu_item::load:
        co_await run_game_with_mode( engine_, planes_, gui_,
                                     StartMode::load{} );
        break;
      case e_main_menu_item::hall_of_fame:
        break;
    }
  }

  wait<maybe<e_main_menu_item>> show_menu() {
    EnumChoiceConfig const config{
      .msg = "[REVOLUTION NOW] Version 0.1.0 -- 19-Dec-25",
      .cancel_actions = { .disallow_clicking_outside = true,
                          .disallow_escape_key       = false },
    };
    refl::enum_map<e_main_menu_item, std::string> names;
    refl::enum_map<e_main_menu_item, bool> disabled;

    using enum e_main_menu_item;
    names[new_random]       = "Start a Game in NEW WORLD";
    names[new_america]      = "Start a Game in AMERICA";
    names[new_customize]    = "CUSTOMIZE New World";
    names[load]             = "LOAD Game";
    names[hall_of_fame]     = "View Hall of Fame";
    disabled[new_random]    = false;
    disabled[new_america]   = false;
    disabled[new_customize] = false;
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
wait<> run_main_menu( IEngine& engine, Planes& planes,
                      IGui& gui ) {
  auto owner        = planes.push();
  PlaneGroup& group = owner.group;

  MainMenuPlane main_menu_plane( engine, planes, gui );
  group.bottom = &main_menu_plane;

  co_await co::while_throws<main_menu_interrupt>(
      [&] { return main_menu_plane.run(); } );
}

} // namespace rn
