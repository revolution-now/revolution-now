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
#include "ss/land-view.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/turn.rds.hpp"
#include "ss/unit-composition.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using unexplored = PlayerSquare::unexplored;
using explored   = PlayerSquare::explored;
using fogged     = FogStatus::fogged;
using clear      = FogStatus::clear;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    set_default_player_type( e_player::french );
    add_player( e_player::french );
    add_player( e_player::english );
    /* no dutch */
    add_player( e_player::spanish );
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
    for( e_player const player_type :
         refl::enum_values<e_player> )
      if( players().players[player_type].has_value() )
        player( player_type )
            .fathers.has[e_founding_father::hernando_de_soto] =
            true;
  }

  void clear_all_fog( gfx::Matrix<PlayerSquare>& m ) {
    for( int y = 0; y < m.size().h; ++y ) {
      for( int x = 0; x < m.size().w; ++x ) {
        Coord const coord{ .x = x, .y = y };
        if( m[coord].holds<unexplored>() ) continue;
        m[coord].emplace<explored>().fog_status.emplace<clear>();
      }
    }
  }

  unique_ptr<IVisibility const> make_viz(
      maybe<e_player> player ) const {
    return create_visibility_for( ss(), player );
  };
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[visibility] unit_visible_squares" ) {
  world W;
  W.create_default_map();
  e_unit_type type = {};
  Coord tile       = {};
  vector<Coord> expected;

  auto f = [&] {
    return unit_visible_squares( W.ss(), W.default_player_type(),
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
  world W;
  W.create_small_map();

  SECTION( "no player" ) {
    VisibilityEntire const viz( W.ss() );

    REQUIRE( viz.player() == nothing );

    // visible.
    REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
             e_tile_visibility::clear );
    REQUIRE( viz.visible( { .x = 1, .y = 0 } ) ==
             e_tile_visibility::clear );
    REQUIRE( viz.visible( { .x = 0, .y = 1 } ) ==
             e_tile_visibility::clear );
    REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
             e_tile_visibility::clear );
    // proto visible.
    REQUIRE( viz.visible( { .x = -1, .y = 0 } ) ==
             e_tile_visibility::clear );
    REQUIRE( viz.visible( { .x = 2, .y = 0 } ) ==
             e_tile_visibility::clear );
    REQUIRE( viz.visible( { .x = 0, .y = -1 } ) ==
             e_tile_visibility::clear );
    REQUIRE( viz.visible( { .x = 1, .y = 2 } ) ==
             e_tile_visibility::clear );
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
    VisibilityForPlayer const viz( W.ss(), e_player::english );

    REQUIRE( viz.player() == e_player::english );

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
    VisibilityForPlayer const viz( W.ss(), e_player::english );

    REQUIRE( viz.player() == e_player::english );

    gfx::Matrix<PlayerSquare>& player_map =
        W.terrain()
            .mutable_player_terrain( e_player::english )
            .map;
    player_map[{ .x = 1, .y = 0 }]
        .emplace<explored>()
        .fog_status.emplace<clear>();
    player_map[{ .x = 0, .y = 0 }]
        .emplace<explored>()
        .fog_status.emplace<clear>();

    // visible.
    REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
             e_tile_visibility::clear );
    REQUIRE( viz.visible( { .x = 1, .y = 0 } ) ==
             e_tile_visibility::clear );
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
    VisibilityForPlayer const viz( W.ss(), e_player::english );

    REQUIRE( viz.player() == e_player::english );

    gfx::Matrix<PlayerSquare>& player_map =
        W.terrain()
            .mutable_player_terrain( e_player::english )
            .map;
    player_map[{ .x = 1, .y = 0 }]
        .emplace<explored>()
        .fog_status.emplace<clear>();
    player_map[{ .x = 0, .y = 0 }]
        .emplace<explored>()
        .fog_status.emplace<fogged>();
    player_map[{ .x = 0, .y = 1 }]
        .emplace<explored>()
        .fog_status.emplace<clear>();

    // visible.
    REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
             e_tile_visibility::fogged );
    REQUIRE( viz.visible( { .x = 1, .y = 0 } ) ==
             e_tile_visibility::clear );
    REQUIRE( viz.visible( { .x = 0, .y = 1 } ) ==
             e_tile_visibility::clear );
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
  world W;

  MockLandViewPlane mock_land_view;
  W.planes().get().set_bottom<ILandViewPlane>( mock_land_view );

  maybe<e_player> revealed;

  auto f = [&] { update_map_visibility( W.ts(), revealed ); };

  REQUIRE( W.map_updater().options().player == nothing );

  mock_land_view.EXPECT__set_visibility( maybe<e_player>{} );
  revealed = nothing;
  f();
  REQUIRE( W.map_updater().options().player == nothing );

  mock_land_view.EXPECT__set_visibility( e_player::spanish );
  revealed = e_player::spanish;
  f();
  REQUIRE( W.map_updater().options().player ==
           e_player::spanish );

  mock_land_view.EXPECT__set_visibility( maybe<e_player>{} );
  revealed = nothing;
  f();
  REQUIRE( W.map_updater().options().player == nothing );

  mock_land_view.EXPECT__set_visibility( e_player::french );
  revealed = e_player::french;
  f();
  REQUIRE( W.map_updater().options().player ==
           e_player::french );
}

TEST_CASE( "[visibility] recompute_fog_for_player" ) {
  world W;
  W.create_default_map();

  auto f = [&] {
    recompute_fog_for_player( W.ss(), W.ts(),
                              e_player::english );
  };

  gfx::Matrix<PlayerSquare>& eng_map =
      W.ss()
          .mutable_terrain_use_with_care
          .mutable_player_terrain( e_player::english )
          .map;
  gfx::Matrix<PlayerSquare>& fr_map =
      W.ss()
          .mutable_terrain_use_with_care
          .mutable_player_terrain( e_player::french )
          .map;

  // Make a checkerboard pattern of visibility.
  for( int y = 0; y < eng_map.size().h; ++y )
    for( int x = 0; x < eng_map.size().w; ++x )
      if( ( x + y ) % 2 == 0 )
        eng_map[{ .x = x, .y = y }]
            .emplace<explored>()
            .fog_status.emplace<fogged>();
  for( int y = 0; y < fr_map.size().h; ++y )
    for( int x = 0; x < fr_map.size().w; ++x )
      if( ( x + y ) % 2 == 0 )
        fr_map[{ .x = x, .y = y }]
            .emplace<explored>()
            .fog_status.emplace<fogged>();

  // Sanity check our checkerboard.
  REQUIRE( eng_map[{ .x = 0, .y = 0 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( fr_map[{ .x = 0, .y = 0 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 1, .y = 0 }] == unexplored{} );
  REQUIRE( fr_map[{ .x = 1, .y = 0 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 0, .y = 1 }] == unexplored{} );
  REQUIRE( fr_map[{ .x = 0, .y = 1 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 1, .y = 1 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( fr_map[{ .x = 1, .y = 1 }]
               .inner_if<explored>()
               .get_if<fogged>() );

  W.clear_all_fog( eng_map );
  W.clear_all_fog( fr_map );

  // Sanity check checkerboard with fog removed.
  REQUIRE( eng_map[{ .x = 0, .y = 0 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( fr_map[{ .x = 0, .y = 0 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 1, .y = 0 }] == unexplored{} );
  REQUIRE( fr_map[{ .x = 1, .y = 0 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 0, .y = 1 }] == unexplored{} );
  REQUIRE( fr_map[{ .x = 0, .y = 1 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 1, .y = 1 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( fr_map[{ .x = 1, .y = 1 }]
               .inner_if<explored>()
               .get_if<clear>() );

  f();

  // Back to all fog.
  REQUIRE( eng_map[{ .x = 0, .y = 0 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( fr_map[{ .x = 0, .y = 0 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 1, .y = 0 }] == unexplored{} );
  REQUIRE( fr_map[{ .x = 1, .y = 0 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 0, .y = 1 }] == unexplored{} );
  REQUIRE( fr_map[{ .x = 0, .y = 1 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 1, .y = 1 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( fr_map[{ .x = 1, .y = 1 }]
               .inner_if<explored>()
               .get_if<clear>() );

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

  REQUIRE( eng_map[{ .x = 0, .y = 0 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 0, .y = 1 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 0, .y = 2 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 0, .y = 3 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 0, .y = 4 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 0, .y = 5 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 0, .y = 6 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 0, .y = 7 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 0, .y = 8 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 0, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 0, .y = 10 }]
               .inner_if<explored>()
               .get_if<clear>() );

  REQUIRE( eng_map[{ .x = 0, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 1, .y = 9 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 2, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 3, .y = 9 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 4, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 5, .y = 9 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 6, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 7, .y = 9 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 8, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 9, .y = 9 }]
               .inner_if<explored>()
               .get_if<clear>() );

  REQUIRE( eng_map[{ .x = 3, .y = 0 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 3, .y = 1 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 3, .y = 2 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 3, .y = 3 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 3, .y = 4 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 3, .y = 5 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 3, .y = 6 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 3, .y = 7 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 3, .y = 8 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 3, .y = 9 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 3, .y = 10 }] == unexplored{} );

  REQUIRE( eng_map[{ .x = 8, .y = 0 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 8, .y = 1 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 8, .y = 2 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 8, .y = 3 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 8, .y = 4 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 8, .y = 5 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 8, .y = 6 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 8, .y = 7 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 8, .y = 8 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 8, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 8, .y = 10 }]
               .inner_if<explored>()
               .get_if<clear>() );

  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 7 }, e_player::english );
  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 2, .y = 2 }, e_player::english );
  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 3, .y = 9 }, e_player::french );
  W.add_unit_on_map( e_unit_type::scout, { .x = 5, .y = 5 },
                     e_player::english );
  W.add_colony( { .x = 7, .y = 2 }, e_player::english );
  W.map_updater().make_squares_visible( e_player::english,
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

  REQUIRE( eng_map[{ .x = 0, .y = 0 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 0, .y = 1 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 0, .y = 2 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 0, .y = 3 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 0, .y = 4 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 0, .y = 5 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 0, .y = 6 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 0, .y = 7 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 0, .y = 8 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 0, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 0, .y = 10 }]
               .inner_if<explored>()
               .get_if<fogged>() );

  REQUIRE( eng_map[{ .x = 0, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 1, .y = 9 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 2, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 3, .y = 9 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 4, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 5, .y = 9 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 6, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 7, .y = 9 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 8, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 9, .y = 9 }]
               .inner_if<explored>()
               .get_if<fogged>() );

  REQUIRE( eng_map[{ .x = 3, .y = 0 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 3, .y = 1 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 3, .y = 2 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 3, .y = 3 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 3, .y = 4 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 3, .y = 5 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 3, .y = 6 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 3, .y = 7 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 3, .y = 8 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 3, .y = 9 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 3, .y = 10 }] == unexplored{} );

  REQUIRE( eng_map[{ .x = 8, .y = 0 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 8, .y = 1 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 8, .y = 2 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 8, .y = 3 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( eng_map[{ .x = 8, .y = 4 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 8, .y = 5 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 8, .y = 6 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 8, .y = 7 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 8, .y = 8 }]
               .inner_if<explored>()
               .get_if<fogged>() );
  REQUIRE( eng_map[{ .x = 8, .y = 9 }] == unexplored{} );
  REQUIRE( eng_map[{ .x = 8, .y = 10 }]
               .inner_if<explored>()
               .get_if<fogged>() );
}

TEST_CASE( "[visibility] should_animate_on_tile" ) {
  world W;
  W.create_small_map();
  Coord const src = { .x = 0, .y = 0 };
  unique_ptr<IVisibility const> viz;

  auto f = [&]() {
    BASE_CHECK( viz != nullptr );
    return should_animate_on_tile( *viz, src );
  };

  viz = W.make_viz( nothing );
  REQUIRE( f() );

  viz = W.make_viz( W.default_player_type() );
  REQUIRE_FALSE( f() );

  W.player_square( { .x = 1, .y = 0 } )
      .emplace<explored>()
      .fog_status.emplace<fogged>();
  REQUIRE_FALSE( f() );

  W.player_square( { .x = 1, .y = 0 } )
      .emplace<explored>()
      .fog_status.emplace<clear>();
  REQUIRE_FALSE( f() );

  W.player_square( src )
      .emplace<explored>()
      .fog_status.emplace<fogged>();
  REQUIRE_FALSE( f() );

  W.player_square( src )
      .emplace<explored>()
      .fog_status.emplace<clear>();
  REQUIRE( f() );

  W.player_square( src )
      .emplace<explored>()
      .fog_status.emplace<fogged>();
  REQUIRE_FALSE( f() );

  W.player_square( src ).emplace<unexplored>();
  REQUIRE_FALSE( f() );

  viz = W.make_viz( nothing );
  REQUIRE( f() );
}

TEST_CASE( "[visibility] should_animate_move" ) {
  world W;
  W.create_small_map();
  Coord const src = { .x = 0, .y = 0 };
  Coord const dst = { .x = 0, .y = 1 };
  unique_ptr<IVisibility const> viz;

  auto f = [&]() {
    BASE_CHECK( viz != nullptr );
    return should_animate_move( *viz, src, dst );
  };

  viz = W.make_viz( nothing );
  REQUIRE( f() );

  viz = W.make_viz( W.default_player_type() );
  REQUIRE_FALSE( f() );

  W.player_square( { .x = 1, .y = 0 } )
      .emplace<explored>()
      .fog_status.emplace<fogged>();
  REQUIRE_FALSE( f() );

  W.player_square( { .x = 1, .y = 0 } )
      .emplace<explored>()
      .fog_status.emplace<clear>();
  REQUIRE_FALSE( f() );

  W.player_square( src )
      .emplace<explored>()
      .fog_status.emplace<fogged>();
  REQUIRE_FALSE( f() );

  W.player_square( dst )
      .emplace<explored>()
      .fog_status.emplace<fogged>();
  REQUIRE_FALSE( f() );

  W.player_square( src )
      .emplace<explored>()
      .fog_status.emplace<clear>();
  REQUIRE( f() );

  W.player_square( dst )
      .emplace<explored>()
      .fog_status.emplace<clear>();
  REQUIRE( f() );

  W.player_square( src )
      .emplace<explored>()
      .fog_status.emplace<fogged>();
  REQUIRE( f() );

  W.player_square( src ).emplace<unexplored>();
  REQUIRE( f() );

  W.player_square( dst )
      .emplace<explored>()
      .fog_status.emplace<fogged>();
  REQUIRE_FALSE( f() );

  W.player_square( dst ).emplace<unexplored>();
  REQUIRE_FALSE( f() );

  viz = W.make_viz( nothing );
  REQUIRE( f() );
}

TEST_CASE(
    "[visibility] does_player_have_fog_removed_on_square" ) {
  world W;
  W.create_small_map();
  Coord const coord = { .x = 0, .y = 0 };
  e_player player   = {};

  auto f = [&] {
    return does_player_have_fog_removed_on_square(
        W.ss(), player, coord );
  };

  // Sanity check.
  REQUIRE_FALSE(
      W.players().players[e_player::dutch].has_value() );

  player = e_player::dutch;
  REQUIRE_FALSE( f() );

  player = e_player::french;
  REQUIRE_FALSE( f() );

  player = e_player::french;
  W.player_square( coord, player )
      .emplace<explored>()
      .fog_status.emplace<fogged>();
  REQUIRE_FALSE( f() );

  player = e_player::french;
  W.player_square( coord, player )
      .emplace<explored>()
      .fog_status.emplace<clear>();
  REQUIRE( f() );
}

TEST_CASE( "[visibility] VisibilityWithOverrides" ) {
  world W;
  W.create_small_map();
  Coord coord;
  Coord const kOutsideCoord = { .x = 2, .y = 2 };
  BASE_CHECK( !W.terrain().square_exists( kOutsideCoord ) );
  VisibilityEntire viz_entire( W.ss() );
  VisibilityForPlayer viz_player( W.ss(), e_player::english );
  VisibilityOverrides overrides;
  // This will keep a reference to the overrides.
  VisibilityWithOverrides viz_overrides_entire(
      W.ss(), viz_entire, overrides );
  VisibilityWithOverrides viz_overrides_player(
      W.ss(), viz_player, overrides );

  gfx::Matrix<PlayerSquare>& player_map =
      W.terrain()
          .mutable_player_terrain( e_player::english )
          .map;

  MapSquare& real_square0 = W.square( { .x = 0, .y = 0 } );
  MapSquare& real_square1 = W.square( { .x = 1, .y = 0 } );
  MapSquare& real_square2 = W.square( { .x = 0, .y = 1 } );
  MapSquare& real_square3 = W.square( { .x = 1, .y = 1 } );
  real_square0            = MapSquare{};
  real_square1            = MapSquare{ .irrigation = true };
  real_square2 =
      MapSquare{ .ground_resource = e_natural_resource::silver };
  real_square3 = MapSquare{ .river = e_river::major };

  player_map[{ .x = 0, .y = 0 }]
      .emplace<explored>()
      .fog_status.emplace<clear>();
  FrozenSquare& frozen_square1 =
      player_map[{ .x = 1, .y = 0 }]
          .emplace<explored>()
          .fog_status.emplace<fogged>()
          .contents;
  frozen_square1 = FrozenSquare{
    .colony   = Colony{},
    .dwelling = Dwelling{ .is_capital = true },
  };

  W.add_tribe( e_tribe::sioux );
  Dwelling const& real_dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::sioux );

  IVisibility* p_viz = nullptr;

  auto visible = [&] { return p_viz->visible( coord ); };

  auto colony_at   = [&] { return p_viz->colony_at( coord ); };
  auto dwelling_at = [&] { return p_viz->dwelling_at( coord ); };
  auto square_at   = [&] { return p_viz->square_at( coord ); };

  SECTION( "no overrides, entire" ) {
    BASE_CHECK( overrides.squares.empty() );
    p_viz = &viz_overrides_entire;
    coord = { .x = 0, .y = 0 };
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == real_square0 );
    coord = { .x = 1, .y = 0 };
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == real_square1 );
    coord = { .x = 0, .y = 1 };
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == real_square2 );
    coord = { .x = 1, .y = 1 };
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == real_dwelling );
    REQUIRE( square_at() == real_square3 );
    coord = kOutsideCoord;
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == MapSquare{} ); // proto.
  }

  SECTION( "no overrides, player" ) {
    BASE_CHECK( overrides.squares.empty() );
    p_viz = &viz_overrides_player;
    coord = { .x = 0, .y = 0 };
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == real_square0 );
    coord = { .x = 1, .y = 0 };
    REQUIRE( visible() == e_tile_visibility::fogged );
    REQUIRE( colony_at() == Colony{} );
    REQUIRE( dwelling_at() == Dwelling{ .is_capital = true } );
    REQUIRE( square_at() == MapSquare{} );
    coord = { .x = 0, .y = 1 };
    REQUIRE( visible() == e_tile_visibility::hidden );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == real_square2 );
    coord = { .x = 1, .y = 1 };
    REQUIRE( visible() == e_tile_visibility::hidden );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == real_square3 );
    coord = kOutsideCoord;
    REQUIRE( visible() == e_tile_visibility::hidden );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == MapSquare{} ); // proto.
  }

  SECTION( "with overrides, entire" ) {
    p_viz = &viz_overrides_entire;
    MapSquare const override_square2{
      .overlay = e_land_overlay::forest };
    MapSquare const override_square3{
      .overlay = e_land_overlay::hills };
    overrides.squares[{ .x = 0, .y = 1 }] = override_square2;
    overrides.squares[{ .x = 1, .y = 1 }] = override_square3;

    overrides.dwellings[{ .x = 1, .y = 0 }] = nothing;
    overrides.dwellings[{ .x = 0, .y = 1 }] =
        Dwelling{ .id = 555 };

    coord = { .x = 0, .y = 0 };
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == real_square0 );
    coord = { .x = 1, .y = 0 };
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == real_square1 );
    coord = { .x = 0, .y = 1 };
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == Dwelling{ .id = 555 } );
    REQUIRE( square_at() == override_square2 );
    coord = { .x = 1, .y = 1 };
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == real_dwelling );
    REQUIRE( square_at() == override_square3 );
    overrides.dwellings[{ .x = 1, .y = 1 }] = nothing;
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == override_square3 );
    coord = kOutsideCoord;
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == MapSquare{} ); // proto.
  }

  SECTION( "with overrides, player" ) {
    p_viz = &viz_overrides_player;
    MapSquare const override_square2{
      .overlay = e_land_overlay::forest };
    MapSquare const override_square3{
      .overlay = e_land_overlay::hills };
    overrides.squares[{ .x = 0, .y = 1 }] = override_square2;
    overrides.squares[{ .x = 1, .y = 1 }] = override_square3;

    overrides.dwellings[{ .x = 0, .y = 1 }] =
        Dwelling{ .id = 555 };

    coord = { .x = 0, .y = 0 };
    REQUIRE( visible() == e_tile_visibility::clear );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == real_square0 );
    coord = { .x = 1, .y = 0 };
    REQUIRE( visible() == e_tile_visibility::fogged );
    REQUIRE( colony_at() == Colony{} );
    REQUIRE( dwelling_at() == Dwelling{ .is_capital = true } );
    REQUIRE( square_at() == MapSquare{} );
    overrides.dwellings[{ .x = 1, .y = 0 }] = nothing;
    REQUIRE( visible() == e_tile_visibility::fogged );
    REQUIRE( colony_at() == Colony{} );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == MapSquare{} );
    coord = { .x = 0, .y = 1 };
    REQUIRE( visible() == e_tile_visibility::hidden );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == Dwelling{ .id = 555 } );
    REQUIRE( square_at() == override_square2 );
    coord = { .x = 1, .y = 1 };
    REQUIRE( visible() == e_tile_visibility::hidden );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == override_square3 );
    coord = kOutsideCoord;
    REQUIRE( visible() == e_tile_visibility::hidden );
    REQUIRE( colony_at() == nothing );
    REQUIRE( dwelling_at() == nothing );
    REQUIRE( square_at() == MapSquare{} ); // proto.
  }
}

TEST_CASE( "[visibility] resource_at" ) {
  world W;
  W.create_small_map();
  Coord const tile{ .x = 0, .y = 1 };
  Coord const kOutsideCoord = { .x = 2, .y = 2 };
  BASE_CHECK( !W.terrain().square_exists( kOutsideCoord ) );

  VisibilityEntire viz_entire( W.ss() );
  VisibilityForPlayer viz_player( W.ss(), e_player::english );
  VisibilityOverrides overrides;
  // These will keep references to `overrides`.
  VisibilityWithOverrides viz_overrides_entire(
      W.ss(), viz_entire, overrides );
  VisibilityWithOverrides viz_overrides_player(
      W.ss(), viz_player, overrides );

  gfx::Matrix<PlayerSquare>& player_map =
      W.terrain()
          .mutable_player_terrain( e_player::english )
          .map;

  MapSquare& real_square = W.square( { .x = 0, .y = 1 } );

  FrozenSquare& frozen_square = player_map[tile]
                                    .emplace<explored>()
                                    .fog_status.emplace<fogged>()
                                    .contents;

  W.add_tribe( e_tribe::cherokee );

  auto const sugar = e_natural_resource::sugar;
  auto const deer  = e_natural_resource::deer;

  SECTION( "no resources" ) {
    REQUIRE( viz_entire.resource_at( tile ) == nothing );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) ==
             nothing );
    REQUIRE( viz_overrides_player.resource_at( tile ) ==
             nothing );
  }

  SECTION( "real ground resource" ) {
    real_square.ground_resource = sugar;
    REQUIRE( viz_entire.resource_at( tile ) == sugar );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) == sugar );
    REQUIRE( viz_overrides_player.resource_at( tile ) ==
             nothing );
  }

  SECTION( "real ground resource, with player vis" ) {
    real_square.ground_resource = sugar;
    frozen_square.square.ground_resource =
        real_square.ground_resource;
    REQUIRE( viz_entire.resource_at( tile ) == sugar );
    REQUIRE( viz_player.resource_at( tile ) == sugar );
    REQUIRE( viz_overrides_entire.resource_at( tile ) == sugar );
    REQUIRE( viz_overrides_player.resource_at( tile ) == sugar );
  }

  SECTION( "real forest resource" ) {
    real_square.forest_resource = deer;
    real_square.overlay         = e_land_overlay::forest;
    REQUIRE( viz_entire.resource_at( tile ) == deer );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) == deer );
    REQUIRE( viz_overrides_player.resource_at( tile ) ==
             nothing );
  }

  SECTION( "real forest resource, with player vis" ) {
    real_square.forest_resource = deer;
    real_square.overlay         = e_land_overlay::forest;
    frozen_square.square.forest_resource =
        real_square.forest_resource;
    frozen_square.square.overlay = real_square.overlay;
    REQUIRE( viz_entire.resource_at( tile ) == deer );
    REQUIRE( viz_player.resource_at( tile ) == deer );
    REQUIRE( viz_overrides_entire.resource_at( tile ) == deer );
    REQUIRE( viz_overrides_player.resource_at( tile ) == deer );
  }

  SECTION( "real ground and forest resource, with ground" ) {
    real_square.ground_resource = sugar;
    real_square.forest_resource = deer;
    REQUIRE( viz_entire.resource_at( tile ) == sugar );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) == sugar );
    REQUIRE( viz_overrides_player.resource_at( tile ) ==
             nothing );
  }

  SECTION( "real ground and forest resource, with forest" ) {
    real_square.ground_resource = sugar;
    real_square.forest_resource = deer;
    real_square.overlay         = e_land_overlay::forest;
    REQUIRE( viz_entire.resource_at( tile ) == deer );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) == deer );
    REQUIRE( viz_overrides_player.resource_at( tile ) ==
             nothing );
  }

  SECTION( "real ground resource under LCR" ) {
    real_square.ground_resource = sugar;
    real_square.lost_city_rumor = true;
    REQUIRE( viz_entire.resource_at( tile ) == nothing );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) ==
             nothing );
    REQUIRE( viz_overrides_player.resource_at( tile ) ==
             nothing );
  }

  SECTION( "real forest resource under LCR" ) {
    real_square.forest_resource = deer;
    real_square.overlay         = e_land_overlay::forest;
    real_square.lost_city_rumor = true;
    REQUIRE( viz_entire.resource_at( tile ) == nothing );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) ==
             nothing );
    REQUIRE( viz_overrides_player.resource_at( tile ) ==
             nothing );
  }

  SECTION( "real ground resource under dwelling" ) {
    real_square.ground_resource = sugar;
    W.add_dwelling( tile, e_tribe::cherokee );
    REQUIRE( viz_entire.resource_at( tile ) == nothing );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) ==
             nothing );
    REQUIRE( viz_overrides_player.resource_at( tile ) ==
             nothing );
  }

  SECTION( "real forest resource under dwelling" ) {
    real_square.forest_resource = deer;
    real_square.overlay         = e_land_overlay::forest;
    W.add_dwelling( tile, e_tribe::cherokee );
    REQUIRE( viz_entire.resource_at( tile ) == nothing );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) ==
             nothing );
    REQUIRE( viz_overrides_player.resource_at( tile ) ==
             nothing );
  }

  SECTION( "real ground resource under override dwelling" ) {
    real_square.ground_resource = sugar;
    overrides.dwellings[tile]   = Dwelling{ .id = 555 };
    REQUIRE( viz_entire.resource_at( tile ) == sugar );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) ==
             nothing );
    REQUIRE( viz_overrides_player.resource_at( tile ) ==
             nothing );
  }

  SECTION( "real forest resource under override dwelling" ) {
    real_square.forest_resource = deer;
    real_square.overlay         = e_land_overlay::forest;
    overrides.dwellings[tile]   = Dwelling{ .id = 555 };
    REQUIRE( viz_entire.resource_at( tile ) == deer );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) ==
             nothing );
    REQUIRE( viz_overrides_player.resource_at( tile ) ==
             nothing );
  }

  SECTION( "real ground resource under LCR, override LCR" ) {
    real_square.ground_resource             = sugar;
    real_square.lost_city_rumor             = true;
    overrides.squares[tile]                 = real_square;
    overrides.squares[tile].lost_city_rumor = false;
    REQUIRE( viz_entire.resource_at( tile ) == nothing );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) == sugar );
    REQUIRE( viz_overrides_player.resource_at( tile ) == sugar );
  }

  SECTION( "real forest resource under LCR, override LCR" ) {
    real_square.forest_resource = deer;
    real_square.overlay         = e_land_overlay::forest;
    real_square.lost_city_rumor = true;
    overrides.squares[tile]     = real_square;
    overrides.squares[tile].lost_city_rumor = false;
    REQUIRE( viz_entire.resource_at( tile ) == nothing );
    REQUIRE( viz_player.resource_at( tile ) == nothing );
    REQUIRE( viz_overrides_entire.resource_at( tile ) == deer );
    REQUIRE( viz_overrides_player.resource_at( tile ) == deer );
  }
}

TEST_CASE( "[visibility] ScopedMapViewer" ) {
  world w;

  MockLandViewPlane mock_land_view;
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );

  using enum e_player;

  w.french().control = e_player_control::human;

  auto& map_revealed = w.land_view().map_revealed;

  // This will cause the french to be the default viewer.
  w.turn().cycle.emplace<TurnCycle::player>().type = french;

  SECTION( "english human" ) {
    w.english().control = e_player_control::human;
    SECTION( "no_special_view/french -> french" ) {
      map_revealed = MapRevealed::no_special_view{};
      {
        ScopedMapViewer const _( w.ss(), w.ts(), french );
        REQUIRE( map_revealed ==
                 MapRevealed::no_special_view{} );
      }
      REQUIRE( map_revealed == MapRevealed::no_special_view{} );
    }

    SECTION( "no_special_view/french -> english" ) {
      map_revealed = MapRevealed::no_special_view{};
      mock_land_view.EXPECT__set_visibility( english );
      mock_land_view.EXPECT__set_visibility( french );
      {
        ScopedMapViewer const _( w.ss(), w.ts(), english );
        REQUIRE( map_revealed ==
                 MapRevealed::player{ .type = english } );
      }
      REQUIRE( map_revealed == MapRevealed::no_special_view{} );
    }

    SECTION( "entire -> french" ) {
      map_revealed = MapRevealed::entire{};
      {
        ScopedMapViewer const _( w.ss(), w.ts(), english );
        REQUIRE( map_revealed == MapRevealed::entire{} );
      }
      REQUIRE( map_revealed == MapRevealed::entire{} );
    }

    SECTION( "entire -> english" ) {
      map_revealed = MapRevealed::entire{};
      {
        ScopedMapViewer const _( w.ss(), w.ts(), english );
        REQUIRE( map_revealed == MapRevealed::entire{} );
      }
      REQUIRE( map_revealed == MapRevealed::entire{} );
    }

    SECTION( "player/french -> french" ) {
      map_revealed = MapRevealed::player{ .type = french };
      {
        ScopedMapViewer const _( w.ss(), w.ts(), french );
        REQUIRE( map_revealed ==
                 MapRevealed::player{ .type = french } );
      }
      REQUIRE( map_revealed ==
               MapRevealed::player{ .type = french } );
    }

    SECTION( "player/french -> english" ) {
      map_revealed = MapRevealed::player{ .type = french };
      mock_land_view.EXPECT__set_visibility( english );
      mock_land_view.EXPECT__set_visibility( french );
      {
        ScopedMapViewer const _( w.ss(), w.ts(), english );
        REQUIRE( map_revealed ==
                 MapRevealed::player{ .type = english } );
      }
      REQUIRE( map_revealed ==
               MapRevealed::player{ .type = french } );
    }
  }

  SECTION( "english not human" ) {
    w.english().control = e_player_control::ai;
    SECTION( "no_special_view/french -> french" ) {
      map_revealed = MapRevealed::no_special_view{};
      {
        ScopedMapViewer const _( w.ss(), w.ts(), french );
        REQUIRE( map_revealed ==
                 MapRevealed::no_special_view{} );
      }
      REQUIRE( map_revealed == MapRevealed::no_special_view{} );
    }

    SECTION( "no_special_view/french -> english" ) {
      map_revealed = MapRevealed::no_special_view{};
      {
        ScopedMapViewer const _( w.ss(), w.ts(), english );
        REQUIRE( map_revealed ==
                 MapRevealed::no_special_view{} );
      }
      REQUIRE( map_revealed == MapRevealed::no_special_view{} );
    }

    SECTION( "entire -> french" ) {
      map_revealed = MapRevealed::entire{};
      {
        ScopedMapViewer const _( w.ss(), w.ts(), english );
        REQUIRE( map_revealed == MapRevealed::entire{} );
      }
      REQUIRE( map_revealed == MapRevealed::entire{} );
    }

    SECTION( "entire -> english" ) {
      map_revealed = MapRevealed::entire{};
      {
        ScopedMapViewer const _( w.ss(), w.ts(), english );
        REQUIRE( map_revealed == MapRevealed::entire{} );
      }
      REQUIRE( map_revealed == MapRevealed::entire{} );
    }

    SECTION( "player/french -> french" ) {
      map_revealed = MapRevealed::player{ .type = french };
      {
        ScopedMapViewer const _( w.ss(), w.ts(), french );
        REQUIRE( map_revealed ==
                 MapRevealed::player{ .type = french } );
      }
      REQUIRE( map_revealed ==
               MapRevealed::player{ .type = french } );
    }

    SECTION( "player/french -> english" ) {
      map_revealed = MapRevealed::player{ .type = french };
      {
        ScopedMapViewer const _( w.ss(), w.ts(), english );
        REQUIRE( map_revealed ==
                 MapRevealed::player{ .type = french } );
      }
      REQUIRE( map_revealed ==
               MapRevealed::player{ .type = french } );
    }
  }
}

} // namespace
} // namespace rn
