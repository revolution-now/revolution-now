/****************************************************************
**show-anim-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-03.
*
* Description: Unit tests for the show-anim module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/show-anim.hpp"

// Testing.
#include "test/fake/world.hpp"

// Revolution Now
#include "src/anim-builder.rds.hpp"
#include "src/imap-updater.hpp"

// ss
#include "src/ss/land-view.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/unit.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_player( e_player::english );
    add_player( e_player::spanish );
    set_default_player_type( e_player::english );
    set_human_player_and_rest_ai( e_player::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, L, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[show-anim] should_animate_tile" ) {
  world w;
  AnimatedTile anim_tile;
  e_player const player = w.default_player_type();

  w.land_view().map_revealed =
      MapRevealed::player{ .type = player };

  auto const f = [&] [[clang::noinline]] {
    return should_animate_tile( w.ss(), anim_tile );
  };

  using enum e_game_menu_option;

  auto& options = w.settings().in_game_options.game_menu_options;

  // Default.
  anim_tile = { .tile = { .x = 0, .y = 0 }, .inhabitants = {} };
  REQUIRE_FALSE( f() );

  anim_tile                  = { .tile        = { .x = 0, .y = 0 },
                                 .inhabitants = { Society::native{} } };
  options[show_indian_moves] = false;
  REQUIRE_FALSE( f() );
  options[show_indian_moves] = true;
  REQUIRE_FALSE( f() );

  anim_tile = {
    .tile        = { .x = 0, .y = 0 },
    .inhabitants = { Society::european{ .player = player } } };
  options[show_foreign_moves] = false;
  REQUIRE_FALSE( f() );
  options[show_foreign_moves] = true;
  REQUIRE_FALSE( f() );

  anim_tile                   = { .tile        = { .x = 0, .y = 0 },
                                  .inhabitants = { Society::european{
                                    .player = e_player::spanish } } };
  options[show_foreign_moves] = false;
  REQUIRE_FALSE( f() );
  options[show_foreign_moves] = true;
  REQUIRE_FALSE( f() );

  w.map_updater().make_squares_visible( player,
                                        { { .x = 0, .y = 0 } } );

  // Default.
  anim_tile = { .tile = { .x = 0, .y = 0 }, .inhabitants = {} };
  REQUIRE_FALSE( f() );

  anim_tile                  = { .tile        = { .x = 0, .y = 0 },
                                 .inhabitants = { Society::native{} } };
  options[show_indian_moves] = false;
  REQUIRE_FALSE( f() );
  options[show_indian_moves] = true;
  REQUIRE( f() );

  anim_tile = {
    .tile        = { .x = 0, .y = 0 },
    .inhabitants = { Society::european{ .player = player } } };
  options[show_foreign_moves] = false;
  REQUIRE( f() );
  options[show_foreign_moves] = true;
  REQUIRE( f() );

  anim_tile                   = { .tile        = { .x = 0, .y = 0 },
                                  .inhabitants = { Society::european{
                                    .player = e_player::spanish } } };
  options[show_foreign_moves] = false;
  REQUIRE_FALSE( f() );
  options[show_foreign_moves] = true;
  REQUIRE( f() );

  w.map_updater().make_squares_fogged( player,
                                       { { .x = 0, .y = 0 } } );

  w.land_view().map_revealed = MapRevealed::entire{};

  // Default.
  anim_tile = { .tile = { .x = 0, .y = 0 }, .inhabitants = {} };
  REQUIRE_FALSE( f() );

  anim_tile                  = { .tile        = { .x = 0, .y = 0 },
                                 .inhabitants = { Society::native{} } };
  options[show_indian_moves] = false;
  REQUIRE_FALSE( f() );
  options[show_indian_moves] = true;
  REQUIRE( f() );

  anim_tile = {
    .tile        = { .x = 0, .y = 0 },
    .inhabitants = { Society::european{ .player = player } } };
  options[show_foreign_moves] = false;
  REQUIRE( f() );
  options[show_foreign_moves] = true;
  REQUIRE( f() );

  anim_tile                   = { .tile        = { .x = 0, .y = 0 },
                                  .inhabitants = { Society::european{
                                    .player = e_player::spanish } } };
  options[show_foreign_moves] = false;
  REQUIRE_FALSE( f() );
  options[show_foreign_moves] = true;
  REQUIRE( f() );
}

TEST_CASE( "[show-anim] should_animate_seq" ) {
  world w;
  AnimationSequence seq;
  e_player const player = w.default_player_type();

  w.land_view().map_revealed =
      MapRevealed::player{ .type = player };

  UnitId const unit_id =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 0, .y = 0 } )
          .id();
  UnitId const foreign_unit_id_1 =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 0, .y = 1 }, e_player::spanish )
          .id();
  UnitId const foreign_unit_id_2 =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 1 }, e_player::spanish )
          .id();

  auto const f = [&] [[clang::noinline]] {
    return should_animate_seq( w.ss(), seq );
  };

  using enum e_game_menu_option;

  auto& options = w.settings().in_game_options.game_menu_options;

  seq = AnimationSequence{
    .sequence = { { AnimationAction{
      .primitive = AnimationPrimitive::hide_unit{
        .unit_id = foreign_unit_id_1 } } } } };
  REQUIRE_FALSE( f() );

  seq = AnimationSequence{
    .sequence = {
      { AnimationAction{ .primitive =
                             AnimationPrimitive::hide_unit{
                               .unit_id = foreign_unit_id_1 } },
        AnimationAction{
          .primitive = AnimationPrimitive::hide_unit{
            .unit_id = foreign_unit_id_2 } } } } };
  REQUIRE_FALSE( f() );

  seq = AnimationSequence{
    .sequence = { { AnimationAction{
      .primitive = AnimationPrimitive::hide_unit{
        .unit_id = unit_id } } } } };
  REQUIRE( f() );

  seq = AnimationSequence{
    .sequence = {
      { AnimationAction{ .primitive =
                             AnimationPrimitive::hide_unit{
                               .unit_id = foreign_unit_id_1 } },
        AnimationAction{ .primitive =
                             AnimationPrimitive::hide_unit{
                               .unit_id = foreign_unit_id_2 } },
        AnimationAction{ .primitive =
                             AnimationPrimitive::hide_unit{
                               .unit_id = unit_id } } } } };
  REQUIRE( f() );

  options[show_foreign_moves] = true;

  seq = AnimationSequence{
    .sequence = {
      { AnimationAction{ .primitive =
                             AnimationPrimitive::hide_unit{
                               .unit_id = foreign_unit_id_1 } },
        AnimationAction{
          .primitive = AnimationPrimitive::hide_unit{
            .unit_id = foreign_unit_id_2 } } } } };
  REQUIRE( f() );
}

} // namespace
} // namespace rn
