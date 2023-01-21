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
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

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
    add_player( e_nation::dutch );
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
      player( nation )
          .fathers.has[e_founding_father::hernando_de_soto] =
          true;
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[visibility] nations_with_visibility_of_square" ) {
  World W;
  W.create_default_map();
  refl::enum_map<e_nation, bool> expected;
  Coord                          tile;

  // . . . . . . . . .
  // . . . . . . . . .
  // . . E . . . . . .
  // . . F . S . . . .
  // . . . . . D . . .
  // . . . . . . . . .
  // . . . . . . . . .
  W.add_unit_on_map( e_unit_type::scout, { .x = 2, .y = 2 },
                     e_nation::english );
  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 2, .y = 3 }, e_nation::french );
  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 4, .y = 3 }, e_nation::spanish );
  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 5, .y = 4 }, e_nation::dutch );

  SECTION( "land only" ) {
    auto f = [&]( Coord tile ) {
      W.square( tile ).surface = e_surface::land;
      return nations_with_visibility_of_square( W.ss(), tile );
    };

    SECTION( "no de soto" ) {
      expected = {
          { e_nation::english, true },
          { e_nation::french, false },
          { e_nation::spanish, true },
          { e_nation::dutch, true },
      };
      tile = { .x = 4, .y = 3 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, true },
          { e_nation::dutch, false },
      };
      tile = { .x = 5, .y = 2 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
          { e_nation::spanish, true },
          { e_nation::dutch, false },
      };
      tile = { .x = 3, .y = 3 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, false },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile = { .x = 0, .y = 0 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, false },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile = { .x = 0, .y = 1 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile = { .x = 7, .y = 5 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile = { .x = 1, .y = 2 };
      REQUIRE( f( tile ) == expected );
    }

    SECTION( "with de soto" ) {
      W.give_de_soto();

      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
          { e_nation::spanish, true },
          { e_nation::dutch, true },
      };
      tile = { .x = 4, .y = 3 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, false },
          { e_nation::spanish, true },
          { e_nation::dutch, true },
      };
      tile = { .x = 5, .y = 2 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
          { e_nation::spanish, true },
          { e_nation::dutch, true },
      };
      tile = { .x = 3, .y = 3 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, false },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile = { .x = 0, .y = 0 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile = { .x = 0, .y = 1 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, false },
          { e_nation::dutch, true },
      };
      tile = { .x = 7, .y = 5 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile = { .x = 1, .y = 2 };
      REQUIRE( f( tile ) == expected );
    }
  }

  SECTION( "water only" ) {
    auto f = [&]( Coord tile ) {
      W.square( tile ).surface = e_surface::water;
      return nations_with_visibility_of_square( W.ss(), tile );
    };

    SECTION( "no de soto" ) {
      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, true },
          { e_nation::dutch, true },
      };
      tile = { .x = 4, .y = 3 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, true },
          { e_nation::dutch, false },
      };
      tile = { .x = 5, .y = 2 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
          { e_nation::spanish, true },
          { e_nation::dutch, false },
      };
      tile = { .x = 3, .y = 3 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile = { .x = 0, .y = 0 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile = { .x = 0, .y = 1 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile = { .x = 7, .y = 5 };
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile = { .x = 1, .y = 2 };
      REQUIRE( f( tile ) == expected );
    }

    SECTION( "with de soto" ) {
      W.give_de_soto();

      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, true },
          { e_nation::dutch, true },
      };
      tile                     = { .x = 4, .y = 3 };
      W.square( tile ).surface = e_surface::water;
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, true },
          { e_nation::dutch, false },
      };
      tile                     = { .x = 5, .y = 2 };
      W.square( tile ).surface = e_surface::water;
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
          { e_nation::spanish, true },
          { e_nation::dutch, false },
      };
      tile                     = { .x = 3, .y = 3 };
      W.square( tile ).surface = e_surface::water;
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile                     = { .x = 0, .y = 0 };
      W.square( tile ).surface = e_surface::water;
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile                     = { .x = 0, .y = 1 };
      W.square( tile ).surface = e_surface::water;
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile                     = { .x = 7, .y = 5 };
      W.square( tile ).surface = e_surface::water;
      REQUIRE( f( tile ) == expected );

      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
          { e_nation::spanish, false },
          { e_nation::dutch, false },
      };
      tile                     = { .x = 1, .y = 2 };
      W.square( tile ).surface = e_surface::water;
      REQUIRE( f( tile ) == expected );
    }
  }
}

TEST_CASE(
    "[visibility] nations_with_visibility_of_square [ships]" ) {
  World W;
  W.create_default_map();
  refl::enum_map<e_nation, bool> expected;
  Coord                          tile;

  // . . . . . . . . .
  // . . . . . . . . .
  // . . E . . . . . .
  // . . F . . . . . .
  // . . . . . . . . .
  // . . . . . . . . .
  W.add_unit_on_map( e_unit_type::galleon, { .x = 2, .y = 2 },
                     e_nation::english );
  W.add_unit_on_map( e_unit_type::caravel, { .x = 2, .y = 3 },
                     e_nation::french );

  SECTION( "land only" ) {
    auto f = [&]( Coord tile ) {
      W.square( tile ).surface = e_surface::land;
      return nations_with_visibility_of_square( W.ss(), tile );
    };

    SECTION( "no de soto" ) {
      // . x . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
      };
      tile = { .x = 1, .y = 0 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . x . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, false },
      };
      tile = { .x = 2, .y = 1 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E x . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
      };
      tile = { .x = 3, .y = 2 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . x . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
      };
      tile = { .x = 4, .y = 3 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . x . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
      };
      tile = { .x = 5, .y = 4 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . x . .
      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
      };
      tile = { .x = 6, .y = 5 };
      REQUIRE( f( tile ) == expected );
    }

    SECTION( "with de soto" ) {
      W.give_de_soto();

      // . x . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
      };
      tile = { .x = 1, .y = 0 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . x . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, false },
      };
      tile = { .x = 2, .y = 1 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E x . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
      };
      tile = { .x = 3, .y = 2 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . x . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
      };
      tile = { .x = 4, .y = 3 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . x . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
      };
      tile = { .x = 5, .y = 4 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . x . .
      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
      };
      tile = { .x = 6, .y = 5 };
      REQUIRE( f( tile ) == expected );
    }
  }

  SECTION( "water only" ) {
    auto f = [&]( Coord tile ) {
      W.square( tile ).surface = e_surface::water;
      return nations_with_visibility_of_square( W.ss(), tile );
    };

    SECTION( "no de soto" ) {
      // . x . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, false },
      };
      tile = { .x = 1, .y = 0 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . x . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, false },
      };
      tile = { .x = 2, .y = 1 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E x . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
      };
      tile = { .x = 3, .y = 2 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . x . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, false },
      };
      tile = { .x = 4, .y = 3 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . x . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
      };
      tile = { .x = 5, .y = 4 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . x . .
      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
      };
      tile = { .x = 6, .y = 5 };
      REQUIRE( f( tile ) == expected );
    }

    SECTION( "with de soto" ) {
      W.give_de_soto();

      // . x . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, false },
      };
      tile = { .x = 1, .y = 0 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . x . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
      };
      tile = { .x = 2, .y = 1 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E x . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
      };
      tile = { .x = 3, .y = 2 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . x . . . .
      // . . . . . . . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, true },
      };
      tile = { .x = 4, .y = 3 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . x . . .
      // . . . . . . . . .
      expected = {
          { e_nation::english, true },
          { e_nation::french, false },
      };
      tile = { .x = 5, .y = 4 };
      REQUIRE( f( tile ) == expected );

      // . . . . . . . . .
      // . . . . . . . . .
      // . . E . . . . . .
      // . . F . . . . . .
      // . . . . . . . . .
      // . . . . . . x . .
      expected = {
          { e_nation::english, false },
          { e_nation::french, false },
      };
      tile = { .x = 6, .y = 5 };
      REQUIRE( f( tile ) == expected );
    }
  }
}

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
    auto viz = Visibility::create( W.ss(), /*nation=*/nothing );

    // visible.
    REQUIRE( viz.visible( { .x = 0, .y = 0 } ) == true );
    REQUIRE( viz.visible( { .x = 1, .y = 0 } ) == true );
    REQUIRE( viz.visible( { .x = 0, .y = 1 } ) == true );
    REQUIRE( viz.visible( { .x = 1, .y = 1 } ) == true );
    // proto visible.
    REQUIRE( viz.visible( { .x = -1, .y = 0 } ) == true );
    REQUIRE( viz.visible( { .x = 2, .y = 0 } ) == true );
    REQUIRE( viz.visible( { .x = 0, .y = -1 } ) == true );
    REQUIRE( viz.visible( { .x = 1, .y = 2 } ) == true );
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
    auto viz = Visibility::create( W.ss(), e_nation::english );

    // visible.
    REQUIRE( viz.visible( { .x = 0, .y = 0 } ) == false );
    REQUIRE( viz.visible( { .x = 1, .y = 0 } ) == false );
    REQUIRE( viz.visible( { .x = 0, .y = 1 } ) == false );
    REQUIRE( viz.visible( { .x = 1, .y = 1 } ) == false );
    // proto visible.
    REQUIRE( viz.visible( { .x = -1, .y = 0 } ) == false );
    REQUIRE( viz.visible( { .x = 2, .y = 0 } ) == false );
    REQUIRE( viz.visible( { .x = 0, .y = -1 } ) == false );
    REQUIRE( viz.visible( { .x = 1, .y = 2 } ) == false );
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

  SECTION( "with player, some visibility" ) {
    auto viz = Visibility::create( W.ss(), e_nation::english );

    Matrix<maybe<FogSquare>>& player_map =
        W.terrain()
            .mutable_player_terrain( e_nation::english )
            .map;
    player_map[{ .x = 1, .y = 0 }].emplace();
    player_map[{ .x = 0, .y = 0 }].emplace();

    // visible.
    REQUIRE( viz.visible( { .x = 0, .y = 0 } ) == true );
    REQUIRE( viz.visible( { .x = 1, .y = 0 } ) == true );
    REQUIRE( viz.visible( { .x = 0, .y = 1 } ) == false );
    REQUIRE( viz.visible( { .x = 1, .y = 1 } ) == false );
    // proto visible.
    REQUIRE( viz.visible( { .x = -1, .y = 0 } ) == false );
    REQUIRE( viz.visible( { .x = 2, .y = 0 } ) == false );
    REQUIRE( viz.visible( { .x = 0, .y = -1 } ) == false );
    REQUIRE( viz.visible( { .x = 1, .y = 2 } ) == false );
    // square_at.
    REQUIRE( viz.square_at( { .x = 0, .y = 0 } ).surface ==
             e_surface::water );
    REQUIRE( viz.square_at( { .x = 1, .y = 0 } ).surface ==
             e_surface::water );
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

  maybe<e_nation>      default_nation;
  maybe<MapRevealed_t> revealed;
  maybe<MapRevealed_t> expected;

  auto f = [&] {
    set_map_visibility( W.planes(), W.ss(), W.ts(), revealed,
                        default_nation );
  };

  expected = nothing;
  REQUIRE( W.land_view().map_revealed == expected );
  REQUIRE( W.map_updater().options().nation == nothing );

  EXPECT_CALL( mock_land_view,
               set_visibility( maybe<e_nation>{} ) );
  revealed       = MapRevealed::entire{};
  default_nation = e_nation::french; // should be irrelevant.
  expected       = revealed;
  f();
  REQUIRE( W.land_view().map_revealed == expected );
  REQUIRE( W.map_updater().options().nation == nothing );

  EXPECT_CALL( mock_land_view,
               set_visibility( e_nation::spanish ) );
  revealed = MapRevealed::nation{ .nation = e_nation::spanish };
  default_nation = e_nation::french; // should be irrelevant.
  expected       = revealed;
  f();
  REQUIRE( W.land_view().map_revealed == expected );
  REQUIRE( W.map_updater().options().nation ==
           e_nation::spanish );

  EXPECT_CALL( mock_land_view,
               set_visibility( e_nation::french ) );
  revealed       = nothing;
  default_nation = e_nation::french;
  expected       = revealed;
  f();
  REQUIRE( W.land_view().map_revealed == expected );
  REQUIRE( W.map_updater().options().nation ==
           e_nation::french );
}

} // namespace
} // namespace rn
