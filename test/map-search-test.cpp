/****************************************************************
**map-search.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-20.
*
* Description: Unit tests for the src/map-search.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/map-search.hpp"

// Testing
#include "test/fake/world.hpp"

// Revolution Now
#include "src/imap-updater.hpp"

// ss
#include "src/ss/colonies.hpp"
#include "src/ss/fog-square.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/tribe.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/range-lite.hpp"
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

namespace rl = ::base::rl;

using ::gfx::point;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_nation::dutch );
    add_player( e_nation::english );
    set_default_player( e_nation::dutch );
    set_default_player_as_human();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, L, L, //
      L, L, L, L, L, //
      L, L, L, L, L, //
      L, L, L, L, L, //
      L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 5 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[map-search] outward_spiral_pythdist_search_existing" ) {
  World  W;
  double max_distance = 0;

  SECTION( "zero distance" ) {
    max_distance      = 0;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{ { .x = 2, .y = 3 } };
    REQUIRE( vec == expected );
  }

  SECTION( "distance=.5" ) {
    max_distance      = .5;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{ { .x = 2, .y = 3 } };
    REQUIRE( vec == expected );
  }

  SECTION( "distance=1" ) {
    max_distance      = 1.0;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{ { .x = 2, .y = 3 },
                                  { .x = 2, .y = 2 },
                                  { .x = 3, .y = 3 },
                                  { .x = 2, .y = 4 },
                                  { .x = 1, .y = 3 } };
    REQUIRE( vec == expected );
  }

  SECTION( "distance=2" ) {
    max_distance      = 2.0;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{
      { .x = 2, .y = 3 }, { .x = 1, .y = 2 },
      { .x = 2, .y = 2 }, { .x = 3, .y = 2 },
      { .x = 3, .y = 3 }, { .x = 3, .y = 4 },
      { .x = 2, .y = 4 }, { .x = 1, .y = 4 },
      { .x = 1, .y = 3 }, { .x = 2, .y = 1 },
      { .x = 4, .y = 3 }, { .x = 0, .y = 3 } };
    REQUIRE( vec == expected );
  }

  SECTION( "distance=3" ) {
    max_distance      = 3.0;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{
      { .x = 2, .y = 3 }, { .x = 1, .y = 2 },
      { .x = 2, .y = 2 }, { .x = 3, .y = 2 },
      { .x = 3, .y = 3 }, { .x = 3, .y = 4 },
      { .x = 2, .y = 4 }, { .x = 1, .y = 4 },
      { .x = 1, .y = 3 }, { .x = 0, .y = 1 },
      { .x = 1, .y = 1 }, { .x = 2, .y = 1 },
      { .x = 3, .y = 1 }, { .x = 4, .y = 1 },
      { .x = 4, .y = 2 }, { .x = 4, .y = 3 },
      { .x = 4, .y = 4 }, { .x = 0, .y = 4 },
      { .x = 0, .y = 3 }, { .x = 0, .y = 2 },
      { .x = 2, .y = 0 } };
    REQUIRE( vec == expected );
  }

  SECTION( "distance=4" ) {
    max_distance      = 4.0;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{
      { .x = 2, .y = 3 }, { .x = 1, .y = 2 }, { .x = 2, .y = 2 },
      { .x = 3, .y = 2 }, { .x = 3, .y = 3 }, { .x = 3, .y = 4 },
      { .x = 2, .y = 4 }, { .x = 1, .y = 4 }, { .x = 1, .y = 3 },
      { .x = 0, .y = 1 }, { .x = 1, .y = 1 }, { .x = 2, .y = 1 },
      { .x = 3, .y = 1 }, { .x = 4, .y = 1 }, { .x = 4, .y = 2 },
      { .x = 4, .y = 3 }, { .x = 4, .y = 4 }, { .x = 0, .y = 4 },
      { .x = 0, .y = 3 }, { .x = 0, .y = 2 }, { .x = 0, .y = 0 },
      { .x = 1, .y = 0 }, { .x = 2, .y = 0 }, { .x = 3, .y = 0 },
      { .x = 4, .y = 0 } };
    REQUIRE( vec == expected );
  }

  SECTION( "max distance" ) {
    max_distance      = 1'000;
    vector<point> gen = outward_spiral_pythdist_search_existing(
        W.ss(), { .x = 2, .y = 3 }, max_distance );
    // Don't cut off the stream just to make sure that it termi-
    // nates on its own.
    vector<point> const vec = rl::all( gen ).to_vector();
    vector<point> const expected{
      { .x = 2, .y = 3 }, { .x = 1, .y = 2 }, { .x = 2, .y = 2 },
      { .x = 3, .y = 2 }, { .x = 3, .y = 3 }, { .x = 3, .y = 4 },
      { .x = 2, .y = 4 }, { .x = 1, .y = 4 }, { .x = 1, .y = 3 },
      { .x = 0, .y = 1 }, { .x = 1, .y = 1 }, { .x = 2, .y = 1 },
      { .x = 3, .y = 1 }, { .x = 4, .y = 1 }, { .x = 4, .y = 2 },
      { .x = 4, .y = 3 }, { .x = 4, .y = 4 }, { .x = 0, .y = 4 },
      { .x = 0, .y = 3 }, { .x = 0, .y = 2 }, { .x = 0, .y = 0 },
      { .x = 1, .y = 0 }, { .x = 2, .y = 0 }, { .x = 3, .y = 0 },
      { .x = 4, .y = 0 } };
    REQUIRE( vec == expected );
  }
}

TEST_CASE( "[map-search] find_any_close_colony" ) {
  World           W;
  Coord           start        = {};
  double          max_distance = {};
  maybe<ColonyId> expected     = {};

  base::function_ref<bool( Colony const& )> const pred_default =
      +[]( Colony const& ) { return true; };
  base::function_ref<bool( Colony const& )> pred = pred_default;

  auto f = [&] {
    return find_any_close_colony( W.ss(), start, max_distance,
                                  pred )
        .fmap(
            []( Colony const& colony ) { return colony.id; } );
  };

  // No colonies, max distance 0.
  start        = { .x = 2, .y = 2 };
  max_distance = 0;
  expected     = nothing;
  REQUIRE( f() == expected );

  // No colonies, max distance 10.
  max_distance = 10;
  expected     = nothing;
  REQUIRE( f() == expected );

  // One colony, max distance 0.
  Colony& colony_1 =
      W.add_colony( { .x = 0, .y = 0 }, e_nation::dutch );
  max_distance = 0;
  expected     = nothing;
  REQUIRE( f() == expected );

  // One colony, max distance 1.
  max_distance = 1;
  expected     = nothing;
  REQUIRE( f() == expected );

  // One colony, max distance 2.
  max_distance = 2;
  expected     = nothing;
  REQUIRE( f() == expected );

  // One colony, max distance 3.
  max_distance = 3;
  expected     = colony_1.id;
  REQUIRE( f() == expected );

  // One colony, max distance 3, with pred, wrong nation.
  max_distance = 3;
  pred         = +[]( Colony const& colony ) {
    return colony.nation == e_nation::english;
  };
  expected = nothing;
  REQUIRE( f() == expected );

  // One colony, max distance 4, with pred, right nation.
  max_distance = 4;
  pred         = +[]( Colony const& colony ) {
    return colony.nation == e_nation::dutch;
  };
  expected = colony_1.id;
  REQUIRE( f() == expected );

  // Two colonies, max distance 5.
  Colony& colony_2 =
      W.add_colony( { .x = 2, .y = 4 }, e_nation::english );
  max_distance = 5;
  pred         = pred_default;
  expected     = colony_2.id;
  REQUIRE( f() == expected );

  // Two colonies, max distance 5, with pred dutch.
  max_distance = 5;
  pred         = +[]( Colony const& colony ) {
    return colony.nation == e_nation::dutch;
  };
  expected = colony_1.id;
  REQUIRE( f() == expected );

  // Three colonies, max distance 0.
  Colony& colony_3 =
      W.add_colony( { .x = 2, .y = 2 }, e_nation::dutch );
  max_distance = 0;
  pred         = pred_default;
  expected     = colony_3.id;
  REQUIRE( f() == expected );

  // Three colonies, max distance 1.
  max_distance = 1;
  pred         = pred_default;
  expected     = colony_3.id;
  REQUIRE( f() == expected );

  // Three colonies, max distance 1, with pred english.
  max_distance = 1;
  pred         = +[]( Colony const& colony ) {
    return colony.nation == e_nation::english;
  };
  expected = nothing;
  REQUIRE( f() == expected );

  // Three colonies, max distance 2, with pred english.
  max_distance = 2;
  pred         = +[]( Colony const& colony ) {
    return colony.nation == e_nation::english;
  };
  expected = colony_2.id;
  REQUIRE( f() == expected );
}

TEST_CASE( "[map-search] find_close_explored_colony" ) {
  World         W;
  maybe<Colony> expected;

  auto f = [&] {
    return find_close_explored_colony(
        W.ss(), W.default_nation(), { .x = 3, .y = 3 }, 3.0 );
  };

  //     0 1 2 3 4
  //   +-----------
  // 0 | a _ b c d
  // 1 | _ 6 _ 7 _
  // 2 | _ _ 2 _ 3
  // 3 | 9 5 _ 1 _
  // 4 | 8 _ _ _ 4

  e_nation nation1 = e_nation::dutch;
  e_nation nation2 = e_nation::english;

  auto add = [&]( Coord coord ) {
    Colony& colony = W.add_colony( coord, nation1 );
    BASE_CHECK( !colony.frozen.has_value() );
    // This makes matching easier in this test.
    colony.buildings = {};
    swap( nation1, nation2 );
    colony.name = std::to_string( colony.id );
  };

  auto make_clear = [&]( Coord coord ) {
    W.map_updater().make_squares_visible( e_nation::dutch,
                                          { coord } );
  };

  auto make_fogged = [&]( Coord coord, bool remove ) {
    W.map_updater().make_squares_visible( e_nation::dutch,
                                          { coord } );
    W.map_updater().make_squares_fogged( e_nation::dutch,
                                         { coord } );
    if( remove ) {
      ColonyId const colony_id =
          W.colonies().from_coord( coord );
      // This is a low-level method, but it is fast and should
      // suffice for this test.
      W.colonies().destroy_colony( colony_id );
    }
  };

  // Visible square with no colony on it, either real or fogged.
  W.map_updater().make_squares_visible( e_nation::dutch,
                                        { { .x = 2, .y = 3 } } );

  add( { .x = 4, .y = 0 } ); // "1", dutch
  add( { .x = 3, .y = 0 } ); // "2", english
  add( { .x = 2, .y = 0 } ); // "3", dutch
  add( { .x = 0, .y = 0 } ); // "4", english
  add( { .x = 0, .y = 3 } ); // "5", dutch
  add( { .x = 0, .y = 4 } ); // "6", english
  add( { .x = 3, .y = 1 } ); // "7", dutch
  add( { .x = 1, .y = 1 } ); // "8", english
  add( { .x = 1, .y = 3 } ); // "9", dutch
  add( { .x = 4, .y = 4 } ); // "10", english
  add( { .x = 4, .y = 2 } ); // "11", dutch
  add( { .x = 2, .y = 2 } ); // "12", english
  add( { .x = 3, .y = 3 } ); // "13", dutch

  expected = nothing;
  REQUIRE( f() == expected );

  make_clear( { .x = 4, .y = 0 } );
  expected = nothing;
  REQUIRE( f() == expected );

  make_fogged( { .x = 3, .y = 0 }, /*remove=*/false );
  expected = Colony{ .id       = 0, // frozen
                     .nation   = e_nation::english,
                     .name     = "2",
                     .location = { .x = 3, .y = 0 },
                     .frozen   = FrozenColony{} };
  REQUIRE( f() == expected );

  make_clear( { .x = 2, .y = 0 } );
  expected = Colony{ .id       = 0, // frozen
                     .nation   = e_nation::english,
                     .name     = "2",
                     .location = { .x = 3, .y = 0 },
                     .frozen   = FrozenColony{} };
  REQUIRE( f() == expected );

  make_fogged( { .x = 0, .y = 0 }, /*remove=*/true );
  expected = Colony{ .id       = 0, // frozen
                     .nation   = e_nation::english,
                     .name     = "2",
                     .location = { .x = 3, .y = 0 },
                     .frozen   = FrozenColony{} };
  REQUIRE( f() == expected );

  make_clear( { .x = 0, .y = 3 } );
  expected = Colony{ .id       = 0, // frozen
                     .nation   = e_nation::english,
                     .name     = "2",
                     .location = { .x = 3, .y = 0 },
                     .frozen   = FrozenColony{} };
  REQUIRE( f() == expected );

  make_fogged( { .x = 0, .y = 4 }, /*remove=*/false );
  expected = Colony{ .id       = 0, // frozen
                     .nation   = e_nation::english,
                     .name     = "2",
                     .location = { .x = 3, .y = 0 },
                     .frozen   = FrozenColony{} };
  REQUIRE( f() == expected );

  make_clear( { .x = 3, .y = 1 } );
  expected = Colony{ .id       = 7,
                     .nation   = e_nation::dutch,
                     .name     = "7",
                     .location = { .x = 3, .y = 1 } };
  REQUIRE( f() == expected );

  make_fogged( { .x = 1, .y = 1 }, /*remove=*/true );
  W.player_square( { .x = 1, .y = 1 } ) = {};

  expected = Colony{ .id       = 7,
                     .nation   = e_nation::dutch,
                     .name     = "7",
                     .location = { .x = 3, .y = 1 } };
  REQUIRE( f() == expected );

  make_fogged( { .x = 1, .y = 1 }, /*remove=*/false );
  expected = Colony{ .id       = 7,
                     .nation   = e_nation::dutch,
                     .name     = "7",
                     .location = { .x = 3, .y = 1 } };
  REQUIRE( f() == expected );

  add( { .x = 1, .y = 1 } ); // english
  make_fogged( { .x = 1, .y = 1 }, /*remove=*/false );
  expected = Colony{ .id       = 0, // frozen
                     .nation   = e_nation::english,
                     .name     = "14",
                     .location = { .x = 1, .y = 1 },
                     .frozen   = FrozenColony{} };
  REQUIRE( f() == expected );

  make_clear( { .x = 1, .y = 3 } );
  expected = Colony{ .id       = 0, // frozen
                     .nation   = e_nation::english,
                     .name     = "14",
                     .location = { .x = 1, .y = 1 },
                     .frozen   = FrozenColony{} };
  REQUIRE( f() == expected );

  make_fogged( { .x = 4, .y = 4 }, /*remove=*/false );
  expected = Colony{ .id       = 0, // frozen
                     .nation   = e_nation::english,
                     .name     = "10",
                     .location = { .x = 4, .y = 4 },
                     .frozen   = FrozenColony{} };
  REQUIRE( f() == expected );

  make_clear( { .x = 4, .y = 2 } );
  expected = Colony{ .id       = 11,
                     .nation   = e_nation::dutch,
                     .name     = "11",
                     .location = { .x = 4, .y = 2 } };
  REQUIRE( f() == expected );

  make_fogged( { .x = 2, .y = 2 }, /*remove=*/true );
  expected = Colony{ .id       = 0, // frozen
                     .nation   = e_nation::english,
                     .name     = "12",
                     .location = { .x = 2, .y = 2 },
                     .frozen   = FrozenColony{} };
  REQUIRE( f() == expected );

  make_clear( { .x = 3, .y = 3 } );
  expected = Colony{ .id       = 13,
                     .nation   = e_nation::dutch,
                     .name     = "13",
                     .location = { .x = 3, .y = 3 } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[map-search] close_friendly_colonies" ) {
  World            W;
  vector<ColonyId> expected;

  auto f = [&] {
    return close_friendly_colonies( W.ss(), W.default_nation(),
                                    { .x = 3, .y = 3 }, 3.0 );
  };

  //     0 1 2 3 4
  //   +-----------
  // 0 | a _ b c d
  // 1 | _ 6 _ 7 _
  // 2 | _ _ 2 _ 3
  // 3 | 9 5 _ 1 _
  // 4 | 8 _ _ _ 4

  constexpr auto D = e_nation::dutch;
  constexpr auto E = e_nation::english;

  auto id1 = W.add_colony( { .x = 3, .y = 3 }, D ).id;
  /*------*/ W.add_colony( { .x = 2, .y = 2 }, E );
  auto id3 = W.add_colony( { .x = 4, .y = 2 }, D ).id;
  /*------*/ W.add_colony( { .x = 4, .y = 4 }, E );
  auto id5 = W.add_colony( { .x = 1, .y = 3 }, D ).id;
  /*------*/ W.add_colony( { .x = 1, .y = 1 }, E );
  auto id7 = W.add_colony( { .x = 3, .y = 1 }, D ).id;
  /*------*/ W.add_colony( { .x = 0, .y = 4 }, E );
  auto id9 = W.add_colony( { .x = 0, .y = 3 }, D ).id;
  /*------*/ W.add_colony( { .x = 0, .y = 0 }, E );
  /*------*/ W.add_colony( { .x = 2, .y = 0 }, D );
  /*------*/ W.add_colony( { .x = 3, .y = 0 }, E );
  /*------*/ W.add_colony( { .x = 4, .y = 0 }, D );

  expected = { id1, id3, id7, id5, id9 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[map-search] find_close_encountered_tribe" ) {
  World            w;
  gfx::point const start    = { .x = 3, .y = 3 };
  double           distance = 0;
  maybe<e_tribe>   expected;
  e_nation const   nation = w.default_nation();

  w.add_tribe( e_tribe::inca );
  w.add_tribe( e_tribe::aztec );
  w.add_tribe( e_tribe::tupi );

  // These should be irrelevant as they are for another nation.
  w.inca().relationship[e_nation::english].encountered  = true;
  w.aztec().relationship[e_nation::english].encountered = true;
  w.tupi().relationship[e_nation::english].encountered  = true;

  // _ D _ _ _
  // _ _ _ _ _
  // _ _ _ _ D
  // _ D _ * _
  // _ _ _ _ _

  auto f = [&] {
    return find_close_encountered_tribe(
        w.ss(), w.default_nation(), start, distance );
  };

  expected = nothing;
  REQUIRE( f() == expected );

  w.tupi().relationship[nation].encountered = true;
  w.add_dwelling( { .x = 1, .y = 0 }, e_tribe::tupi );
  distance = 4;
  expected = e_tribe::tupi;
  REQUIRE( f() == expected );

  distance = 3;
  expected = nothing;
  REQUIRE( f() == expected );

  w.tupi().relationship[nation].encountered = false;
  distance                                  = 100;
  expected                                  = nothing;
  REQUIRE( f() == expected );

  w.tupi().relationship[nation].encountered = true;
  distance                                  = 100;
  expected                                  = e_tribe::tupi;
  REQUIRE( f() == expected );

  w.tupi().relationship[nation].encountered = true;
  w.add_dwelling( { .x = 1, .y = 3 }, e_tribe::aztec );
  distance = 100;
  expected = e_tribe::tupi;
  REQUIRE( f() == expected );

  w.inca().relationship[nation].encountered  = true;
  w.aztec().relationship[nation].encountered = true;
  distance                                   = 100;
  expected                                   = e_tribe::aztec;
  REQUIRE( f() == expected );

  w.inca().relationship[nation].encountered = false;
  w.add_dwelling( { .x = 4, .y = 2 }, e_tribe::inca );
  distance = 1;
  expected = nothing;
  REQUIRE( f() == expected );

  distance = 2;
  expected = e_tribe::aztec;
  REQUIRE( f() == expected );

  w.inca().relationship[nation].encountered = true;
  distance                                  = 100;
  expected                                  = e_tribe::inca;
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
