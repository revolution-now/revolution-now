/****************************************************************
**game-options-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-01.
*
* Description: Unit tests for the game-options module.
*
*****************************************************************/
#include "settings.rds.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/game-options.hpp"

// Testing.
#include "test/fake/world.hpp"

// ss
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      _, L, _, //
      L, L, L, //
      _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[game-options] disable_game_option" ) {
  World W;

  auto& game_menu_options =
      W.ss().settings.in_game_options.game_menu_options;

  e_game_menu_option option = {};
  using E                   = e_game_menu_option;

  auto f = [&] {
    return disable_game_option( W.ss(), W.ts(), option );
  };

  option = E::autosave;
  REQUIRE( f() == false );
  option = E::combat_analysis;
  REQUIRE( f() == false );
  option = E::end_of_turn;
  REQUIRE( f() == false );
  option = E::fast_piece_slide;
  REQUIRE( f() == false );
  option = E::show_fog_of_war;
  REQUIRE( f() == false );
  option = E::show_foreign_moves;
  REQUIRE( f() == false );
  option = E::show_indian_moves;
  REQUIRE( f() == false );
  option = E::tutorial_hints;
  REQUIRE( f() == false );
  option = E::water_color_cycling;
  REQUIRE( f() == false );

  game_menu_options[E::autosave]            = true;
  game_menu_options[E::end_of_turn]         = true;
  game_menu_options[E::show_fog_of_war]     = true;
  game_menu_options[E::show_indian_moves]   = true;
  game_menu_options[E::water_color_cycling] = true;

  option = E::autosave;
  REQUIRE( f() == true );
  option = E::combat_analysis;
  REQUIRE( f() == false );
  option = E::end_of_turn;
  REQUIRE( f() == true );
  option = E::fast_piece_slide;
  REQUIRE( f() == false );
  option = E::show_fog_of_war;
  REQUIRE( f() == true );
  option = E::show_foreign_moves;
  REQUIRE( f() == false );
  option = E::show_indian_moves;
  REQUIRE( f() == true );
  option = E::tutorial_hints;
  REQUIRE( f() == false );
  option = E::water_color_cycling;
  REQUIRE( f() == true );
}

} // namespace
} // namespace rn
