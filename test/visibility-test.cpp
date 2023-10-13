/****************************************************************
**visibility.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-30.
*
* Description: Unit tests for the src/visibility.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/visibility.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/land-view-plane.hpp"

// Revolution Now
#include "src/imap-updater.hpp"
#include "src/plane-stack.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/unit-composer.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    set_default_player( e_nation::french );
    add_player( e_nation::french );
    add_player( e_nation::english );
    /* no dutch */
    add_player( e_nation::spanish );
  }

  void create_default_map() {
    MapSquare const L = make_grassland();
    MapSquare const _ = make_ocean();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _, _, L, _, L, L, L, L, L, L, _, L, L,
      L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, _, L, L, L, L, L, L, L, L, L,
      _, L, _, _, L, _, L, _, L, L, L, L, _, L, L,
      L, L, L, L, L, L, L, L, _, L, L, L, L, L, L,
      _, _, _, L, L, _, _, _, _, _, L, L, L, L, L,
      _, L, _, _, L, _, L, _, _, _, _, L, _, L, L,
      L, L, L, L, L, L, L, _, _, _, _, L, L, L, L,
      _, L, L, L, L, _, L, L, L, _, L, L, L, L, L,
      _, L, _, _, L, _, L, _, L, L, L, L, _, L, L,
      L, L, L, L, L, L, L, _, _, L, L, L, L, L, L,
      _, L, L, L, L, _, L, L, L, L, L, L, L, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 15 );
  }

  void create_small_map() {
    MapSquare const L = make_grassland();
    MapSquare const _ = make_ocean();
    // clang-format off
    vector<MapSquare> tiles{
      _, L,
      L, _,
    };
    // clang-format on
    build_map( std::move( tiles ), 2 );
    terrain()
        .mutable_proto_square( e_cardinal_direction::n )
        .surface = e_surface::land;
    terrain()
        .mutable_proto_square( e_cardinal_direction::e )
        .surface = e_surface::water;
    terrain()
        .mutable_proto_square( e_cardinal_direction::w )
        .surface = e_surface::water;
    terrain()
        .mutable_proto_square( e_cardinal_direction::s )
        .surface = e_surface::land;
  }

  void give_de_soto() {
    for( e_nation nation : refl::enum_values<e_nation> )
      if( players().players[nation].has_value() )
        player( nation )
            .fathers.has[e_founding_father::hernando_de_soto] =
            true;
  }

  void clear_all_fog( gfx::Matrix<maybe<FogSquare>>& m ) {
    for( int y = 0; y < m.size().h; ++y ) {
      for( int x = 0; x < m.size().w; ++x ) {
        Coord const coord{ .x = x, .y = y };
        if( !m[coord].has_value() ) continue;
        m[coord]->fog_of_war_removed = true;
      }
    }
  }

  unique_ptr<IVisibility const> make_viz(
      maybe<e_nation> nation ) const {
    return create_visibility_for( ss(), nation );
  };
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[visibility] unit_visible_squares" ) {
  World W;
  W.create_default_map();
  e_unit_type   type = {};
  Coord         tile = {};
  vector<Coord> expected;

  auto f = [&] {
    return unit_visible_squares( W.ss(), W.default_nation(),
                                 type, tile );
  };

  // _, L, _, _, L, _, L, L, L, L, L, L, _, L, L,
  // L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
  // _, L, L, L, L, _, L, L, L, L, L, L, L, L, L,
  // _, L, _, _, L, _, L, _, L, L, L, L, _, L, L,
  // L, L, L, L, L, L, L, L, _, L, L, L, L, L, L,
  // _, _, _, L, L, _, _, _, _, _, L, L, L, L, L,
  // _, L, _, _, L, _, L, _, _, _, _, L, _, L, L,

  tile = { .x = 3, .y = 3 };
  type = e_unit_type::scout;
  // clang-format off
  expected = {
    {.x=1,.y=1},  {.x=2, .y=1},  {.x=3, .y=1},  {.x=4, .y=1},  {.x=5, .y=1},
    {.x=1,.y=2},  {.x=2, .y=2},  {.x=3, .y=2},  {.x=4, .y=2},/*{.x=5, .y=2},*/
    {.x=1,.y=3},  {.x=2, .y=3},  {.x=3, .y=3},  {.x=4, .y=3},/*{.x=5, .y=3},*/
    {.x=1,.y=4},  {.x=2, .y=4},  {.x=3, .y=4},  {.x=4, .y=4},  {.x=5, .y=4},
  /*{.x=1,.y=5},  {.x=2, .y=5},*/{.x=3, .y=5},  {.x=4, .y=5},/*{.x=5, .y=5},*/
  };
  // clang-format on
  REQUIRE( f() == expected );

  tile = { .x = 3, .y = 3 };
  type = e_unit_type::galleon;
  // clang-format off
  expected = {
  /*{.x=1,.y=1},  {.x=2, .y=1},  {.x=3, .y=1},  {.x=4, .y=1},  {.x=5, .y=1},*/
  /*{.x=1,.y=2},*/{.x=2, .y=2},  {.x=3, .y=2},  {.x=4, .y=2},  {.x=5, .y=2},
  /*{.x=1,.y=3},*/{.x=2, .y=3},  {.x=3, .y=3},  {.x=4, .y=3},  {.x=5, .y=3},
  /*{.x=1,.y=4},*/{.x=2, .y=4},  {.x=3, .y=4},  {.x=4, .y=4},/*{.x=5, .y=4},*/
    {.x=1,.y=5},  {.x=2, .y=5},/*{.x=3, .y=5},  {.x=4, .y=5},*/{.x=5, .y=5},
  };
  // clang-format on
  REQUIRE( f() == expected );

  tile = { .x = 3, .y = 3 };
  type = e_unit_type::free_colonist;
  // clang-format off
  expected = {
  /*{.x=1,.y=1},  {.x=2, .y=1},  {.x=3, .y=1},  {.x=4, .y=1},  {.x=5, .y=1},*/
  /*{.x=1,.y=2},*/{.x=2, .y=2},  {.x=3, .y=2},  {.x=4, .y=2},/*{.x=5, .y=2},*/
  /*{.x=1,.y=3},*/{.x=2, .y=3},  {.x=3, .y=3},  {.x=4, .y=3},/*{.x=5, .y=3},*/
  /*{.x=1,.y=4},*/{.x=2, .y=4},  {.x=3, .y=4},  {.x=4, .y=4},/*{.x=5, .y=4},*/
  /*{.x=1,.y=5},  {.x=2, .y=5},  {.x=3, .y=5},  {.x=4, .y=5},  {.x=5, .y=5},*/
  };
  // clang-format on
  REQUIRE( f() == expected );

  tile = { .x = 3, .y = 3 };
  type = e_unit_type::caravel;
  // clang-format off
  expected = {
  /*{.x=1,.y=1},  {.x=2, .y=1},  {.x=3, .y=1},  {.x=4, .y=1},  {.x=5, .y=1},*/
  /*{.x=1,.y=2},*/{.x=2, .y=2},  {.x=3, .y=2},  {.x=4, .y=2},/*{.x=5, .y=2},*/
  /*{.x=1,.y=3},*/{.x=2, .y=3},  {.x=3, .y=3},  {.x=4, .y=3},/*{.x=5, .y=3},*/
  /*{.x=1,.y=4},*/{.x=2, .y=4},  {.x=3, .y=4},  {.x=4, .y=4},/*{.x=5, .y=4},*/
  /*{.x=1,.y=5},  {.x=2, .y=5},  {.x=3, .y=5},  {.x=4, .y=5},  {.x=5, .y=5},*/
  };
  // clang-format on
  REQUIRE( f() == expected );

  W.give_de_soto();

  tile = { .x = 3, .y = 3 };
  type = e_unit_type::free_colonist;
  // clang-format off
  expected = {
    {.x=1,.y=1},  {.x=2, .y=1},  {.x=3, .y=1},  {.x=4, .y=1},  {.x=5, .y=1},
    {.x=1,.y=2},  {.x=2, .y=2},  {.x=3, .y=2},  {.x=4, .y=2},/*{.x=5, .y=2},*/
    {.x=1,.y=3},  {.x=2, .y=3},  {.x=3, .y=3},  {.x=4, .y=3},/*{.x=5, .y=3},*/
    {.x=1,.y=4},  {.x=2, .y=4},  {.x=3, .y=4},  {.x=4, .y=4},  {.x=5, .y=4},
  /*{.x=1,.y=5},  {.x=2, .y=5},*/{.x=3, .y=5},  {.x=4, .y=5},/*{.x=5, .y=5},*/
  };
  // clang-format on
  REQUIRE( f() == expected );

  tile = { .x = 3, .y = 3 };
  type = e_unit_type::caravel;
  // clang-format off
  expected = {
  /*{.x=1,.y=1},  {.x=2, .y=1},  {.x=3, .y=1},  {.x=4, .y=1},  {.x=5, .y=1},*/
  /*{.x=1,.y=2},*/{.x=2, .y=2},  {.x=3, .y=2},  {.x=4, .y=2},  {.x=5, .y=2},
  /*{.x=1,.y=3},*/{.x=2, .y=3},  {.x=3, .y=3},  {.x=4, .y=3},  {.x=5, .y=3},
  /*{.x=1,.y=4},*/{.x=2, .y=4},  {.x=3, .y=4},  {.x=4, .y=4},/*{.x=5, .y=4},*/
    {.x=1,.y=5},  {.x=2, .y=5},/*{.x=3, .y=5},  {.x=4, .y=5},*/{.x=5, .y=5},
  };
  // clang-format on
  REQUIRE( f() == expected );
}

TEST_CASE( "[visibility] Visibility" ) {
  World W;
  W.create_small_map();

  SECTION( "no player" ) {
    VisibilityEntire const viz( W.ss() );

    REQUIRE( viz.nation() == nothing );

    // visible.
    REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 1, .y = 0 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 0, .y = 1 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
             e_tile_visibility::visible_and_clear );
    // proto visible.
    REQUIRE( viz.visible( { .x = -1, .y = 0 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 2, .y = 0 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 0, .y = -1 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 1, .y = 2 } ) ==
             e_tile_visibility::visible_and_clear );
    // square_at.
    REQUIRE( viz.square_at( { .x = 0, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 1, .y = 0 } ).surface ==
             e_surface::land );
    REQUIRE( viz.square_at( { .x = 0, .y = 1 } ).surface ==
             e_surface::land );
    REQUIRE( viz.square_at( { .x = 1, .y = 1 } ).surface ==
             e_surface::water );
    // proto square at.
    REQUIRE( viz.square_at( { .x = -1, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 2, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 0, .y = -1 } ).surface ==
             e_surface::land );
    REQUIRE( viz.square_at( { .x = 1, .y = 2 } ).surface ==
             e_surface::land );
  }

  SECTION( "with player, no visibility" ) {
    VisibilityForNation const viz( W.ss(), e_nation::english );

    REQUIRE( viz.nation() == e_nation::english );

    // visible.
    REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 1, .y = 0 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 0, .y = 1 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
             e_tile_visibility::hidden );
    // proto visible.
    REQUIRE( viz.visible( { .x = -1, .y = 0 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 2, .y = 0 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 0, .y = -1 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 1, .y = 2 } ) ==
             e_tile_visibility::hidden );
    // square_at.
    REQUIRE( viz.square_at( { .x = 0, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 1, .y = 0 } ).surface ==
             e_surface::land );
    REQUIRE( viz.square_at( { .x = 0, .y = 1 } ).surface ==
             e_surface::land );
    REQUIRE( viz.square_at( { .x = 1, .y = 1 } ).surface ==
             e_surface::water );
    // proto square at.
    REQUIRE( viz.square_at( { .x = -1, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 2, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 0, .y = -1 } ).surface ==
             e_surface::land );
    REQUIRE( viz.square_at( { .x = 1, .y = 2 } ).surface ==
             e_surface::land );
  }

  SECTION( "with player, some visibility, no fog" ) {
    VisibilityForNation const viz( W.ss(), e_nation::english );

    REQUIRE( viz.nation() == e_nation::english );

    gfx::Matrix<maybe<FogSquare>>& player_map =
        W.terrain()
            .mutable_player_terrain( e_nation::english )
            .map;
    player_map[{ .x = 1, .y = 0 }].emplace().fog_of_war_removed =
        true;
    player_map[{ .x = 0, .y = 0 }].emplace().fog_of_war_removed =
        true;

    // visible.
    REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 1, .y = 0 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 0, .y = 1 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
             e_tile_visibility::hidden );
    // proto visible.
    REQUIRE( viz.visible( { .x = -1, .y = 0 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 2, .y = 0 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 0, .y = -1 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 1, .y = 2 } ) ==
             e_tile_visibility::hidden );
    // square_at.
    REQUIRE( viz.square_at( { .x = 0, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 1, .y = 0 } ).surface ==
             e_surface::land );
    REQUIRE( viz.square_at( { .x = 0, .y = 1 } ).surface ==
             e_surface::land );
    REQUIRE( viz.square_at( { .x = 1, .y = 1 } ).surface ==
             e_surface::water );
    // proto square at.
    REQUIRE( viz.square_at( { .x = -1, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 2, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 0, .y = -1 } ).surface ==
             e_surface::land );
    REQUIRE( viz.square_at( { .x = 1, .y = 2 } ).surface ==
             e_surface::land );
  }

  SECTION( "with player, some visibility, some fog" ) {
    VisibilityForNation const viz( W.ss(), e_nation::english );

    REQUIRE( viz.nation() == e_nation::english );

    gfx::Matrix<maybe<FogSquare>>& player_map =
        W.terrain()
            .mutable_player_terrain( e_nation::english )
            .map;
    player_map[{ .x = 1, .y = 0 }] =
        FogSquare{ .square = W.square( { .x = 1, .y = 0 } ),
                   .fog_of_war_removed = true };
    player_map[{ .x = 0, .y = 0 }].emplace();
    player_map[{ .x = 0, .y = 1 }] =
        FogSquare{ .square = W.square( { .x = 0, .y = 1 } ),
                   .fog_of_war_removed = true };

    // visible.
    REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
             e_tile_visibility::visible_with_fog );
    REQUIRE( viz.visible( { .x = 1, .y = 0 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 0, .y = 1 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
             e_tile_visibility::hidden );
    // proto visible.
    REQUIRE( viz.visible( { .x = -1, .y = 0 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 2, .y = 0 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 0, .y = -1 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 1, .y = 2 } ) ==
             e_tile_visibility::hidden );
    // square_at.
    REQUIRE( viz.square_at( { .x = 0, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 1, .y = 0 } ).surface ==
             e_surface::land );
    REQUIRE( viz.square_at( { .x = 0, .y = 1 } ).surface ==
             e_surface::land );
    REQUIRE( viz.square_at( { .x = 1, .y = 1 } ).surface ==
             e_surface::water );
    // proto square at.
    REQUIRE( viz.square_at( { .x = -1, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 2, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 0, .y = -1 } ).surface ==
             e_surface::land );
    REQUIRE( viz.square_at( { .x = 1, .y = 2 } ).surface ==
             e_surface::land );
  }
}

TEST_CASE( "[visibility] set_map_visibility" ) {
  World W;

  MockLandViewPlane mock_land_view;
  W.planes().back().land_view = &mock_land_view;

  maybe<e_nation> revealed;

  auto f = [&] { update_map_visibility( W.ts(), revealed ); };

  REQUIRE( W.map_updater().options().nation == nothing );

  mock_land_view.EXPECT__set_visibility( maybe<e_nation>{} );
  revealed = nothing;
  f();
  REQUIRE( W.map_updater().options().nation == nothing );

  mock_land_view.EXPECT__set_visibility( e_nation::spanish );
  revealed = e_nation::spanish;
  f();
  REQUIRE( W.map_updater().options().nation ==
           e_nation::spanish );

  mock_land_view.EXPECT__set_visibility( maybe<e_nation>{} );
  revealed = nothing;
  f();
  REQUIRE( W.map_updater().options().nation == nothing );

  mock_land_view.EXPECT__set_visibility( e_nation::french );
  revealed = e_nation::french;
  f();
  REQUIRE( W.map_updater().options().nation ==
           e_nation::french );
}

TEST_CASE( "[visibility] recompute_fog_for_nation" ) {
  World W;
  W.create_default_map();

  auto f = [&] {
    recompute_fog_for_nation( W.ss(), W.ts(),
                              e_nation::english );
  };

  gfx::Matrix<maybe<FogSquare>>& eng_map =
      W.ss()
          .mutable_terrain_use_with_care
          .mutable_player_terrain( e_nation::english )
          .map;
  gfx::Matrix<maybe<FogSquare>>& fr_map =
      W.ss()
          .mutable_terrain_use_with_care
          .mutable_player_terrain( e_nation::french )
          .map;

  // Make a checkerboard pattern of visibility.
  for( int y = 0; y < eng_map.size().h; ++y )
    for( int x = 0; x < eng_map.size().w; ++x )
      if( ( x + y ) % 2 == 0 )
        eng_map[{ .x = x, .y = y }].emplace();
  for( int y = 0; y < fr_map.size().h; ++y )
    for( int x = 0; x < fr_map.size().w; ++x )
      if( ( x + y ) % 2 == 0 )
        fr_map[{ .x = x, .y = y }].emplace();

  // Sanity check our checkerboard.
  REQUIRE( eng_map[{ .x = 0, .y = 0 }].has_value() );
  REQUIRE( fr_map[{ .x = 0, .y = 0 }].has_value() );
  REQUIRE( !eng_map[{ .x = 1, .y = 0 }].has_value() );
  REQUIRE( !fr_map[{ .x = 1, .y = 0 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 1 }].has_value() );
  REQUIRE( !fr_map[{ .x = 0, .y = 1 }].has_value() );
  REQUIRE( eng_map[{ .x = 1, .y = 1 }].has_value() );
  REQUIRE( fr_map[{ .x = 1, .y = 1 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 0 }]->fog_of_war_removed );
  REQUIRE( !fr_map[{ .x = 0, .y = 0 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 1, .y = 1 }]->fog_of_war_removed );
  REQUIRE( !fr_map[{ .x = 1, .y = 1 }]->fog_of_war_removed );

  W.clear_all_fog( eng_map );
  W.clear_all_fog( fr_map );

  // Sanity check checkerboard with fog removed.
  REQUIRE( eng_map[{ .x = 0, .y = 0 }].has_value() );
  REQUIRE( fr_map[{ .x = 0, .y = 0 }].has_value() );
  REQUIRE( !eng_map[{ .x = 1, .y = 0 }].has_value() );
  REQUIRE( !fr_map[{ .x = 1, .y = 0 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 1 }].has_value() );
  REQUIRE( !fr_map[{ .x = 0, .y = 1 }].has_value() );
  REQUIRE( eng_map[{ .x = 1, .y = 1 }].has_value() );
  REQUIRE( fr_map[{ .x = 1, .y = 1 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 0 }]->fog_of_war_removed );
  REQUIRE( fr_map[{ .x = 0, .y = 0 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 1, .y = 1 }]->fog_of_war_removed );
  REQUIRE( fr_map[{ .x = 1, .y = 1 }]->fog_of_war_removed );

  f();

  // Back to all fog.
  REQUIRE( eng_map[{ .x = 0, .y = 0 }].has_value() );
  REQUIRE( fr_map[{ .x = 0, .y = 0 }].has_value() );
  REQUIRE( !eng_map[{ .x = 1, .y = 0 }].has_value() );
  REQUIRE( !fr_map[{ .x = 1, .y = 0 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 1 }].has_value() );
  REQUIRE( !fr_map[{ .x = 0, .y = 1 }].has_value() );
  REQUIRE( eng_map[{ .x = 1, .y = 1 }].has_value() );
  REQUIRE( fr_map[{ .x = 1, .y = 1 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 0 }]->fog_of_war_removed );
  REQUIRE( fr_map[{ .x = 0, .y = 0 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 1, .y = 1 }]->fog_of_war_removed );
  REQUIRE( fr_map[{ .x = 1, .y = 1 }]->fog_of_war_removed );

  W.clear_all_fog( eng_map );

  // . . . . . . . . . .
  // . . . . . . . . . .
  // . . f . . . . c . .
  // . . . . . . . . . .
  // . . . . . . . . . .
  // . . . . . s . . . .
  // . . . . . . . . . .
  // f . . . . . . . . .
  // . . . . . . . . . .
  // . . . x . . . . . .
  // . . . . . . . . . .

  REQUIRE( eng_map[{ .x = 0, .y = 0 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 1 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 2 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 3 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 4 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 5 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 6 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 7 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 8 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 10 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 0 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 0, .y = 2 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 0, .y = 4 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 0, .y = 6 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 0, .y = 8 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 0, .y = 10 }]->fog_of_war_removed );

  REQUIRE( !eng_map[{ .x = 0, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 1, .y = 9 }].has_value() );
  REQUIRE( !eng_map[{ .x = 2, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 9 }].has_value() );
  REQUIRE( !eng_map[{ .x = 4, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 5, .y = 9 }].has_value() );
  REQUIRE( !eng_map[{ .x = 6, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 7, .y = 9 }].has_value() );
  REQUIRE( !eng_map[{ .x = 8, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 9, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 1, .y = 9 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 3, .y = 9 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 5, .y = 9 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 7, .y = 9 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 9, .y = 9 }]->fog_of_war_removed );

  REQUIRE( !eng_map[{ .x = 3, .y = 0 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 1 }].has_value() );
  REQUIRE( !eng_map[{ .x = 3, .y = 2 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 3 }].has_value() );
  REQUIRE( !eng_map[{ .x = 3, .y = 4 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 5 }].has_value() );
  REQUIRE( !eng_map[{ .x = 3, .y = 6 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 7 }].has_value() );
  REQUIRE( !eng_map[{ .x = 3, .y = 8 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 9 }].has_value() );
  REQUIRE( !eng_map[{ .x = 3, .y = 10 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 1 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 3, .y = 3 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 3, .y = 5 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 3, .y = 7 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 3, .y = 9 }]->fog_of_war_removed );

  REQUIRE( eng_map[{ .x = 8, .y = 0 }].has_value() );
  REQUIRE( !eng_map[{ .x = 8, .y = 1 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 2 }].has_value() );
  REQUIRE( !eng_map[{ .x = 8, .y = 3 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 4 }].has_value() );
  REQUIRE( !eng_map[{ .x = 8, .y = 5 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 6 }].has_value() );
  REQUIRE( !eng_map[{ .x = 8, .y = 7 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 8 }].has_value() );
  REQUIRE( !eng_map[{ .x = 8, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 10 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 0 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 8, .y = 2 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 8, .y = 4 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 8, .y = 6 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 8, .y = 8 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 8, .y = 10 }]->fog_of_war_removed );

  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 7 }, e_nation::english );
  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 2, .y = 2 }, e_nation::english );
  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 3, .y = 9 }, e_nation::french );
  W.add_unit_on_map( e_unit_type::scout, { .x = 5, .y = 5 },
                     e_nation::english );
  W.add_colony( { .x = 7, .y = 2 }, e_nation::english );
  W.map_updater().make_squares_visible( e_nation::english,
                                        { { .x = 7, .y = 2 },
                                          { .x = 6, .y = 1 },
                                          { .x = 7, .y = 1 },
                                          { .x = 8, .y = 1 },
                                          { .x = 6, .y = 2 },
                                          { .x = 7, .y = 2 },
                                          { .x = 8, .y = 2 },
                                          { .x = 6, .y = 3 },
                                          { .x = 7, .y = 3 },
                                          { .x = 8, .y = 3 } } );

  f();

  REQUIRE( eng_map[{ .x = 0, .y = 0 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 1 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 2 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 3 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 4 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 5 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 6 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 7 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 8 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 0, .y = 10 }].has_value() );
  REQUIRE( !eng_map[{ .x = 0, .y = 0 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 0, .y = 2 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 0, .y = 4 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 0, .y = 6 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 0, .y = 7 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 0, .y = 8 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 0, .y = 10 }]->fog_of_war_removed );

  REQUIRE( !eng_map[{ .x = 0, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 1, .y = 9 }].has_value() );
  REQUIRE( !eng_map[{ .x = 2, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 9 }].has_value() );
  REQUIRE( !eng_map[{ .x = 4, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 5, .y = 9 }].has_value() );
  REQUIRE( !eng_map[{ .x = 6, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 7, .y = 9 }].has_value() );
  REQUIRE( !eng_map[{ .x = 8, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 9, .y = 9 }].has_value() );
  REQUIRE( !eng_map[{ .x = 1, .y = 9 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 3, .y = 9 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 5, .y = 9 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 7, .y = 9 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 9, .y = 9 }]->fog_of_war_removed );

  REQUIRE( !eng_map[{ .x = 3, .y = 0 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 1 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 2 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 3 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 4 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 5 }].has_value() );
  REQUIRE( !eng_map[{ .x = 3, .y = 6 }].has_value() ); // water.
  REQUIRE( eng_map[{ .x = 3, .y = 7 }].has_value() );
  REQUIRE( !eng_map[{ .x = 3, .y = 8 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 9 }].has_value() );
  REQUIRE( !eng_map[{ .x = 3, .y = 10 }].has_value() );
  REQUIRE( eng_map[{ .x = 3, .y = 1 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 3, .y = 2 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 3, .y = 3 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 3, .y = 4 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 3, .y = 5 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 3, .y = 7 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 3, .y = 9 }]->fog_of_war_removed );

  REQUIRE( eng_map[{ .x = 8, .y = 0 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 1 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 2 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 3 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 4 }].has_value() );
  REQUIRE( !eng_map[{ .x = 8, .y = 5 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 6 }].has_value() );
  REQUIRE( !eng_map[{ .x = 8, .y = 7 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 8 }].has_value() );
  REQUIRE( !eng_map[{ .x = 8, .y = 9 }].has_value() );
  REQUIRE( eng_map[{ .x = 8, .y = 10 }].has_value() );
  REQUIRE( !eng_map[{ .x = 8, .y = 0 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 8, .y = 1 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 8, .y = 2 }]->fog_of_war_removed );
  REQUIRE( eng_map[{ .x = 8, .y = 3 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 8, .y = 4 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 8, .y = 6 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 8, .y = 8 }]->fog_of_war_removed );
  REQUIRE( !eng_map[{ .x = 8, .y = 10 }]->fog_of_war_removed );
}

TEST_CASE( "[visibility] fog_square_at" ) {
  World W;
  W.create_small_map();
  maybe<FogSquare> expected;
  Coord            coord;
  Coord const      kOutsideCoord = { .x = 2, .y = 2 };
  BASE_CHECK( !W.terrain().square_exists( kOutsideCoord ) );
  unique_ptr<IVisibility const> viz;

  auto f = [&] { return viz->fog_square_at( coord ); };

  gfx::Matrix<maybe<FogSquare>>& player_map =
      W.terrain()
          .mutable_player_terrain( e_nation::english )
          .map;

  FogSquare& fog_square1 =
      player_map[{ .x = 0, .y = 0 }].emplace();
  fog_square1 = FogSquare{ .square = MapSquare{ .road = true },
                           .fog_of_war_removed = true };
  FogSquare& fog_square2 =
      player_map[{ .x = 1, .y = 0 }].emplace();
  fog_square2 = FogSquare{
      .colony   = FogColony{},
      .dwelling = FogDwelling{ .capital = true },
  };

  // No nation.
  viz = W.make_viz( nothing );

  coord = { .x = 0, .y = 0 };
  REQUIRE( f() == nothing );
  coord = { .x = 1, .y = 0 };
  REQUIRE( f() == nothing );
  coord = { .x = 0, .y = 1 };
  REQUIRE( f() == nothing );
  coord = { .x = 1, .y = 1 };
  REQUIRE( f() == nothing );
  coord = kOutsideCoord;
  REQUIRE( f() == nothing );

  // English.
  viz = W.make_viz( e_nation::english );

  coord = { .x = 0, .y = 0 };
  REQUIRE( f() == fog_square1 );
  coord = { .x = 1, .y = 0 };
  REQUIRE( f() == fog_square2 );
  coord = { .x = 0, .y = 1 };
  REQUIRE( f() == nothing );
  coord = { .x = 1, .y = 1 };
  REQUIRE( f() == nothing );
  coord = kOutsideCoord;
  REQUIRE( f() == nothing );
}

TEST_CASE( "[visibility] should_animate_move" ) {
  World W;
  W.create_small_map();
  Coord const                   src = { .x = 0, .y = 0 };
  Coord const                   dst = { .x = 0, .y = 1 };
  unique_ptr<IVisibility const> viz;

  auto f = [&]() {
    BASE_CHECK( viz != nullptr );
    return should_animate_move( *viz, src, dst );
  };

  viz = W.make_viz( nothing );
  REQUIRE( f() );

  viz = W.make_viz( W.default_nation() );
  REQUIRE_FALSE( f() );

  W.player_square( { .x = 1, .y = 0 } ).emplace();
  REQUIRE_FALSE( f() );

  W.player_square( { .x = 1, .y = 0 } )
      .emplace()
      .fog_of_war_removed = true;
  REQUIRE_FALSE( f() );

  W.player_square( src ).emplace();
  REQUIRE_FALSE( f() );

  W.player_square( dst ).emplace();
  REQUIRE_FALSE( f() );

  W.player_square( src ).emplace().fog_of_war_removed = true;
  REQUIRE( f() );

  W.player_square( dst ).emplace().fog_of_war_removed = true;
  REQUIRE( f() );

  W.player_square( src ).emplace().fog_of_war_removed = false;
  REQUIRE( f() );

  W.player_square( src ).reset();
  REQUIRE( f() );

  W.player_square( dst ).emplace().fog_of_war_removed = false;
  REQUIRE_FALSE( f() );

  W.player_square( dst ).reset();
  REQUIRE_FALSE( f() );

  viz = W.make_viz( nothing );
  REQUIRE( f() );
}

TEST_CASE(
    "[visibility] does_nation_have_fog_removed_on_square" ) {
  World W;
  W.create_small_map();
  Coord const coord  = { .x = 0, .y = 0 };
  e_nation    nation = {};

  auto f = [&] {
    return does_nation_have_fog_removed_on_square(
        W.ss(), nation, coord );
  };

  // Sanity check.
  REQUIRE_FALSE(
      W.players().players[e_nation::dutch].has_value() );

  nation = e_nation::dutch;
  REQUIRE_FALSE( f() );

  nation = e_nation::french;
  REQUIRE_FALSE( f() );

  nation = e_nation::french;
  W.player_square( coord, nation ).emplace();
  REQUIRE_FALSE( f() );

  nation = e_nation::french;
  W.player_square( coord, nation ).emplace().fog_of_war_removed =
      true;
  REQUIRE( f() );
}

} // namespace
} // namespace rn
