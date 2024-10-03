/****************************************************************
**map-updater.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-04.
*
* Description: Unit tests for the src/map-updater.* module.
*
*****************************************************************/
// Under test.
#include "src/map-updater.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "src/ss/ref.hpp"
#include "src/ss/terrain.hpp"

// refl
#include "src/refl/to-str.hpp" // IWYU pragma: keep

// base
#include "src/base/to-str-ext-std.hpp" // IWYU pragma: keep

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

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
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_nation::dutch );
    add_player( e_nation::french );
    set_default_player( e_nation::dutch );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
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
TEST_CASE(
    "[map-updater] "
    "NonRenderingMapUpdater::make_squares_fogged" ) {
  World                  W;
  NonRenderingMapUpdater map_updater( W.ss() );
  vector<BuffersUpdated> expected_buffers;
  e_nation const         nation = W.default_nation();
  PlayerTerrain&         player_terrain =
      W.ss()
          .mutable_terrain_use_with_care.mutable_player_terrain(
              nation );
  Coord const tile = { .x = 1, .y = 1 };

  MapSquare&   real_square = W.square( tile );
  PlayerSquare expected_fog_square;

  auto f = [&] {
    return map_updater.make_squares_fogged( nation, { tile } );
  };

  auto frozen_square = [&]() -> PlayerSquare& {
    return player_terrain.map[tile];
  };

  // Initially totally not visible.
  REQUIRE( frozen_square() == unexplored{} );

  // On hidden.
  expected_buffers = {
    { .tile = tile, .landscape = false, .obfuscation = false } };
  expected_fog_square = unexplored{};
  REQUIRE( f() == expected_buffers );
  REQUIRE( frozen_square() == expected_fog_square );

  // Now make it visible.
  frozen_square()
      .emplace<explored>()
      .fog_status.emplace<clear>();
  expected_buffers = {
    { .tile = tile, .landscape = false, .obfuscation = true } };
  expected_fog_square.emplace<explored>()
      .fog_status.emplace<fogged>()
      .contents.square = real_square;
  REQUIRE( f() == expected_buffers );
  REQUIRE( frozen_square() == expected_fog_square );

  // Now change the real square and verify no update.
  BASE_CHECK( !real_square.road );
  real_square.road = true;
  expected_buffers = {
    { .tile = tile, .landscape = false, .obfuscation = false } };
  expected_fog_square.emplace<explored>()
      .fog_status.emplace<fogged>()
      .contents.square = real_square;
  expected_fog_square.get<explored>()
      .fog_status.get<fogged>()
      .contents.square.road = false;
  REQUIRE( f() == expected_buffers );
  REQUIRE( frozen_square() == expected_fog_square );

  // Remove fog and try again.
  frozen_square()
      .emplace<explored>()
      .fog_status.emplace<clear>();
  // landscape is false here because the make_squares_fogged
  // method assumes that, if the fog was removed, then the tile
  // has already been rendered in its actual state.
  expected_buffers = {
    { .tile = tile, .landscape = false, .obfuscation = true } };
  BASE_CHECK( real_square.road );
  expected_fog_square.emplace<explored>()
      .fog_status.emplace<fogged>()
      .contents.square = real_square;
  REQUIRE( f() == expected_buffers );
  REQUIRE( frozen_square() == expected_fog_square );

  // Try again but with fog rendering disabled.
  {
    auto _ = map_updater.push_options_and_redraw(
        []( auto& options ) {
          options.render_fog_of_war = false;
        } );
    frozen_square()
        .emplace<explored>()
        .fog_status.emplace<clear>();

    expected_buffers = { { .tile      = tile,
                           .landscape = false,
                           // The difference is here.
                           .obfuscation = false } };

    expected_fog_square.emplace<explored>()
        .fog_status.emplace<fogged>()
        .contents.square = real_square;
    BASE_CHECK( real_square.road );
    REQUIRE( f() == expected_buffers );
    REQUIRE( frozen_square() == expected_fog_square );
  }
}

TEST_CASE(
    "[map-updater] "
    "NonRenderingMapUpdater::make_squares_visible" ) {
  World                  W;
  NonRenderingMapUpdater map_updater( W.ss() );
  vector<BuffersUpdated> expected_buffers;
  e_nation               nation = W.default_nation();
  PlayerTerrain&         player_terrain =
      W.ss()
          .mutable_terrain_use_with_care.mutable_player_terrain(
              nation );
  Coord const tile = { .x = 1, .y = 1 };

  MapSquare&   real_square = W.square( tile );
  PlayerSquare expected_fog_square;

  auto f = [&] {
    return map_updater.make_squares_visible( nation, { tile } );
  };

  auto frozen_square = [&]() -> PlayerSquare& {
    return player_terrain.map[tile];
  };

  // Initially totally not visible.
  REQUIRE( frozen_square() == unexplored{} );

  // On hidden.
  nation           = W.default_nation();
  expected_buffers = {
    { .tile = tile, .landscape = true, .obfuscation = true } };
  expected_fog_square.emplace<explored>()
      .fog_status.emplace<clear>();
  REQUIRE( f() == expected_buffers );
  REQUIRE( frozen_square() == expected_fog_square );

  // On hidden for non-existent nation.
  nation = e_nation::french;
  BASE_CHECK( nation != W.default_nation() );
  expected_buffers = {
    { .tile = tile, .landscape = true, .obfuscation = true } };
  expected_fog_square.emplace<explored>()
      .fog_status.emplace<clear>();
  REQUIRE( f() == expected_buffers );
  REQUIRE( frozen_square() == expected_fog_square );

  // On visible and clear.
  nation = W.default_nation();
  frozen_square()
      .emplace<explored>()
      .fog_status.emplace<clear>();
  expected_buffers = {
    { .tile = tile, .landscape = false, .obfuscation = false } };
  expected_fog_square.emplace<explored>()
      .fog_status.emplace<clear>();
  REQUIRE( f() == expected_buffers );
  REQUIRE( frozen_square() == expected_fog_square );

  // On fogged.
  nation = W.default_nation();
  frozen_square()
      .emplace<explored>()
      .fog_status.emplace<fogged>()
      .contents.square = real_square;
  expected_buffers     = {
    { .tile = tile, .landscape = false, .obfuscation = true } };
  expected_fog_square.emplace<explored>()
      .fog_status.emplace<clear>();
  REQUIRE( f() == expected_buffers );
  REQUIRE( frozen_square() == expected_fog_square );

  // On fogged with map update.
  nation = W.default_nation();
  frozen_square()
      .emplace<explored>()
      .fog_status.emplace<fogged>()
      .contents.square = real_square;
  real_square.road     = true;
  expected_buffers     = {
    { .tile = tile, .landscape = true, .obfuscation = true } };
  expected_fog_square.emplace<explored>()
      .fog_status.emplace<clear>();
  BASE_CHECK( real_square.road == true );
  REQUIRE( f() == expected_buffers );
  REQUIRE( frozen_square() == expected_fog_square );
}

TEST_CASE(
    "[map-updater] NonRenderingMapUpdater::modify_map_square" ) {
  World                  W;
  NonRenderingMapUpdater map_updater( W.ss() );
  e_nation const         nation = e_nation::dutch;
  BuffersUpdated         expected_buffers;
  auto*                  mutator = +[]( MapSquare& ) {};
  Coord const            tile    = { .x = 1, .y = 1 };

  MapSquare& real_square = W.square( tile );
  MapSquare  expected_real_square;

  auto f = [&] {
    return map_updater.modify_map_square( tile, mutator );
  };

  auto frozen_square = [&]() -> PlayerSquare& {
    PlayerTerrain& player_terrain =
        W.ss()
            .mutable_terrain_use_with_care
            .mutable_player_terrain( nation );
    return player_terrain.map[tile];
  };

  // Initially totally not visible.
  REQUIRE( frozen_square() == unexplored{} );

  // No-op, with no nation.
  {
    expected_buffers = {
      .tile = tile, .landscape = false, .obfuscation = false };
    expected_real_square = real_square;
    mutator              = +[]( MapSquare& ) {};
    REQUIRE( f() == expected_buffers );
    REQUIRE( real_square == expected_real_square );
    REQUIRE( frozen_square() == unexplored{} );
  }

  // No-op, with nation.
  {
    auto _ = map_updater.push_options_and_redraw(
        []( auto& options ) {
          options.nation = e_nation::dutch;
        } );
    expected_buffers = {
      .tile = tile, .landscape = false, .obfuscation = false };
    expected_real_square = real_square;
    mutator              = +[]( MapSquare& ) {};
    REQUIRE( f() == expected_buffers );
    REQUIRE( real_square == expected_real_square );
    REQUIRE( frozen_square() == unexplored{} );
  }

  // Add road, with no nation. The landscape buffer should always
  // get updated here since the whole map is visible and clear.
  {
    REQUIRE( map_updater.options().nation == nothing );
    expected_buffers = {
      .tile = tile, .landscape = true, .obfuscation = false };
    expected_real_square      = real_square;
    expected_real_square.road = !expected_real_square.road;
    mutator =
        +[]( MapSquare& square ) { square.road = !square.road; };
    REQUIRE( f() == expected_buffers );
    REQUIRE( real_square == expected_real_square );
    REQUIRE( frozen_square() == unexplored{} );
  }

  // Remove road, with nation. The landscape buffer should not
  // get updated here becauase the nation has no visibility.
  {
    auto _ = map_updater.push_options_and_redraw(
        []( auto& options ) {
          options.nation = e_nation::dutch;
        } );
    REQUIRE( map_updater.options().nation == e_nation::dutch );
    expected_buffers = {
      .tile = tile, .landscape = false, .obfuscation = false };
    expected_real_square      = real_square;
    expected_real_square.road = !expected_real_square.road;
    mutator =
        +[]( MapSquare& square ) { square.road = !square.road; };
    REQUIRE( f() == expected_buffers );
    REQUIRE( real_square == expected_real_square );
    REQUIRE( frozen_square() == unexplored{} );
  }

  // Add road, with nation that has explored but still fog.
  {
    auto _ = map_updater.push_options_and_redraw(
        []( auto& options ) {
          options.nation = e_nation::dutch;
        } );
    REQUIRE( map_updater.options().nation == e_nation::dutch );
    expected_buffers = {
      .tile = tile, .landscape = false, .obfuscation = false };
    frozen_square()
        .emplace<explored>()
        .fog_status.emplace<fogged>();
    expected_real_square      = real_square;
    expected_real_square.road = !expected_real_square.road;
    mutator =
        +[]( MapSquare& square ) { square.road = !square.road; };
    REQUIRE( f() == expected_buffers );
    REQUIRE( real_square == expected_real_square );
    REQUIRE(
        frozen_square().inner_if<explored>().get_if<fogged>() );
  }

  // Remove road, with nation that has clear visibility.
  {
    auto _ = map_updater.push_options_and_redraw(
        []( auto& options ) {
          options.nation = e_nation::dutch;
        } );
    REQUIRE( map_updater.options().nation == e_nation::dutch );
    expected_buffers = {
      .tile = tile, .landscape = true, .obfuscation = false };
    BASE_CHECK( !frozen_square().holds<unexplored>() );
    frozen_square()
        .emplace<explored>()
        .fog_status.emplace<clear>();
    expected_real_square      = real_square;
    expected_real_square.road = !expected_real_square.road;
    mutator =
        +[]( MapSquare& square ) { square.road = !square.road; };
    REQUIRE( f() == expected_buffers );
    REQUIRE( real_square == expected_real_square );
    REQUIRE(
        frozen_square().inner_if<explored>().get_if<clear>() );
  }
}

TEST_CASE(
    "[map-updater] "
    "NonRenderingMapUpdater::modify_entire_square" ) {
  World                  W;
  NonRenderingMapUpdater map_updater( W.ss() );
  Coord const            tile = { .x = 1, .y = 1 };

  MapSquare& real_square = W.square( tile );
  MapSquare  expected_real_square;

  auto f = [&]( auto&& mutator ) {
    map_updater.modify_entire_map_no_redraw( mutator );
  };

  auto frozen_square = [&]( e_nation nation ) -> PlayerSquare& {
    PlayerTerrain& player_terrain =
        W.ss()
            .mutable_terrain_use_with_care
            .mutable_player_terrain( nation );
    return player_terrain.map[tile];
  };

  // Initially totally not visible.
  REQUIRE( frozen_square( e_nation::dutch ) == unexplored{} );
  REQUIRE( frozen_square( e_nation::french ) == unexplored{} );

  // No-op, with no nation.
  expected_real_square = real_square;
  f( []( auto& ) {} );
  REQUIRE( real_square == expected_real_square );
  REQUIRE( frozen_square( e_nation::dutch ) == unexplored{} );
  REQUIRE( frozen_square( e_nation::french ) == unexplored{} );

  // Add road.
  expected_real_square      = real_square;
  expected_real_square.road = !expected_real_square.road;
  f( [&]( auto& real_terrain ) {
    real_terrain.map[tile].road = !real_terrain.map[tile].road;
  } );
  REQUIRE( real_square == expected_real_square );
  REQUIRE( frozen_square( e_nation::dutch ) == unexplored{} );
  REQUIRE( frozen_square( e_nation::french ) == unexplored{} );
}

// This test case will do some additional (possibly redundant)
// testing on the transitioning of tiles to and from various
// states of visible, hidden, and fog.
TEST_CASE( "[map-updater] fog of war" ) {
  World                  W;
  NonRenderingMapUpdater map_updater( W.ss() );
  vector<BuffersUpdated> expected;
  e_nation const         nation = W.default_nation();
  PlayerTerrain&         player_terrain =
      W.ss()
          .mutable_terrain_use_with_care.mutable_player_terrain(
              nation );
  Coord const coord1 = { .x = 0, .y = 0 };
  Coord const coord2 = { .x = 1, .y = 0 };

  REQUIRE( player_terrain.map[coord1] == unexplored{} );
  REQUIRE( player_terrain.map[coord2] == unexplored{} );

  {
    expected = {
      { .tile = coord1, .landscape = true, .obfuscation = true },
      { .tile        = coord2,
        .landscape   = true,
        .obfuscation = true } };
    REQUIRE( map_updater.make_squares_visible(
                 nation, { coord1, coord2 } ) == expected );
    REQUIRE( !player_terrain.map[coord1].holds<unexplored>() );
    REQUIRE( !player_terrain.map[coord2].holds<unexplored>() );
    PlayerSquare const& fog_square1 = player_terrain.map[coord1];
    PlayerSquare const& fog_square2 = player_terrain.map[coord2];
    REQUIRE( fog_square1.inner_if<explored>().get_if<clear>() );
    REQUIRE( fog_square2.inner_if<explored>().get_if<clear>() );
  }

  {
    expected = { { .tile        = coord1,
                   .landscape   = false,
                   .obfuscation = true },
                 { .tile        = coord2,
                   .landscape   = false,
                   .obfuscation = true } };
    REQUIRE( map_updater.make_squares_fogged(
                 nation, { coord1, coord2 } ) == expected );
    REQUIRE( !player_terrain.map[coord1].holds<unexplored>() );
    REQUIRE( !player_terrain.map[coord2].holds<unexplored>() );
    PlayerSquare const& fog_square1 = player_terrain.map[coord1];
    PlayerSquare const& fog_square2 = player_terrain.map[coord2];
    REQUIRE( fog_square1.inner_if<explored>().get_if<fogged>() );
    REQUIRE( fog_square2.inner_if<explored>().get_if<fogged>() );
  }

  {
    expected = { { .tile        = coord1,
                   .landscape   = false,
                   .obfuscation = true } };
    REQUIRE( map_updater.make_squares_visible(
                 nation, { coord1 } ) == expected );
    REQUIRE( !player_terrain.map[coord1].holds<unexplored>() );
    REQUIRE( !player_terrain.map[coord2].holds<unexplored>() );
    PlayerSquare const& fog_square1 = player_terrain.map[coord1];
    PlayerSquare const& fog_square2 = player_terrain.map[coord2];
    REQUIRE( fog_square1.inner_if<explored>().get_if<clear>() );
    REQUIRE( fog_square2.inner_if<explored>().get_if<fogged>() );
  }

  {
    expected = {
      { .tile = coord1, .landscape = true, .obfuscation = true },
      { .tile        = coord2,
        .landscape   = false,
        .obfuscation = true } };

    player_terrain.map[coord1] = unexplored{};
    REQUIRE( map_updater.make_squares_visible(
                 nation, { coord1, coord2 } ) == expected );
    REQUIRE( !player_terrain.map[coord1].holds<unexplored>() );
    REQUIRE( !player_terrain.map[coord2].holds<unexplored>() );
    PlayerSquare const& fog_square1 = player_terrain.map[coord1];
    PlayerSquare const& fog_square2 = player_terrain.map[coord2];
    REQUIRE( fog_square1.inner_if<explored>().get_if<clear>() );
    REQUIRE( fog_square2.inner_if<explored>().get_if<clear>() );
  }

  {
    expected = { { .tile        = coord1,
                   .landscape   = false,
                   .obfuscation = false },
                 { .tile        = coord2,
                   .landscape   = false,
                   .obfuscation = false } };
    REQUIRE( map_updater.make_squares_visible(
                 nation, { coord1, coord2 } ) == expected );
    REQUIRE( !player_terrain.map[coord1].holds<unexplored>() );
    REQUIRE( !player_terrain.map[coord2].holds<unexplored>() );
    PlayerSquare const& fog_square1 = player_terrain.map[coord1];
    PlayerSquare const& fog_square2 = player_terrain.map[coord2];
    REQUIRE( fog_square1.inner_if<explored>().get_if<clear>() );
    REQUIRE( fog_square2.inner_if<explored>().get_if<clear>() );
  }

  {
    expected = { { .tile        = coord1,
                   .landscape   = false,
                   .obfuscation = false },
                 { .tile        = coord2,
                   .landscape   = false,
                   .obfuscation = false } };
    REQUIRE( map_updater.make_squares_visible(
                 nation, { coord1, coord2 } ) == expected );
    REQUIRE( !player_terrain.map[coord1].holds<unexplored>() );
    REQUIRE( !player_terrain.map[coord2].holds<unexplored>() );
    PlayerSquare const& fog_square1 = player_terrain.map[coord1];
    PlayerSquare const& fog_square2 = player_terrain.map[coord2];
    REQUIRE( fog_square1.inner_if<explored>().get_if<clear>() );
    REQUIRE( fog_square2.inner_if<explored>().get_if<clear>() );
  }

  {
    expected = { { .tile        = coord1,
                   .landscape   = false,
                   .obfuscation = true } };
    REQUIRE( map_updater.make_squares_fogged(
                 nation, { coord1 } ) == expected );
    expected = { { .tile        = coord1,
                   .landscape   = false,
                   .obfuscation = true },
                 { .tile        = coord2,
                   .landscape   = false,
                   .obfuscation = false } };
    REQUIRE( map_updater.make_squares_visible(
                 nation, { coord1, coord2 } ) == expected );
    REQUIRE( !player_terrain.map[coord1].holds<unexplored>() );
    REQUIRE( !player_terrain.map[coord2].holds<unexplored>() );
    PlayerSquare const& fog_square1 = player_terrain.map[coord1];
    PlayerSquare const& fog_square2 = player_terrain.map[coord2];
    REQUIRE( fog_square1.inner_if<explored>().get_if<clear>() );
    REQUIRE( fog_square2.inner_if<explored>().get_if<clear>() );
  }
}

TEST_CASE(
    "[map-updater] "
    "NonRenderingMapUpdater::force_redraw_tiles" ) {
  World                  W;
  NonRenderingMapUpdater map_updater( W.ss() );
  vector<BuffersUpdated> expected;
  Coord const            coord1 = { .x = 0, .y = 0 };
  Coord const            coord2 = { .x = 1, .y = 0 };

  expected = {};
  REQUIRE( map_updater.force_redraw_tiles(
               { coord1, coord2 } ) == expected );
}

} // namespace
} // namespace rn
