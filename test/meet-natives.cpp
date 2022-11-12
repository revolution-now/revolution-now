/****************************************************************
**meet-natives.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-09.
*
* Description: Unit tests for the src/meet-natives.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/meet-natives.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "ss/natives.hpp"
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
    add_player( e_nation::english );
    set_default_player( e_nation::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _, L, L, L,
      L, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 6 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[meet-natives] check_meet_tribes" ) {
  World             W;
  Coord             square;
  vector<MeetTribe> expected;

  auto f = [&] {
    return check_meet_tribes( W.ss(), W.default_player(),
                              square );
  };

  SECTION( "nothing" ) {
    square   = { .x = 0, .y = 1 };
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one dwelling too far" ) {
    W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    square = { .x = 1, .y = 3 };
    CHECK( W.terrain().square_at( square ).surface ==
           e_surface::land );
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one dwelling meet" ) {
    W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    W.add_dwelling( { .x = 4, .y = 4 }, e_tribe::inca );
    square   = { .x = 0, .y = 1 };
    expected = {
        MeetTribe{
            .is_first      = true,
            .tribe         = e_tribe::inca,
            .num_dwellings = 2,
            .land_awarded  = {},
        },
    };
    REQUIRE( f() == expected );
  }

  SECTION( "one dwelling already encountered" ) {
    W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    W.add_dwelling( { .x = 4, .y = 4 }, e_tribe::inca );
    square      = { .x = 0, .y = 1 };
    Tribe& inca = W.natives().tribe_for( e_tribe::inca );
    inca.relationship[W.default_nation()].emplace();
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one dwelling from water no meet" ) {
    W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    W.add_dwelling( { .x = 4, .y = 4 }, e_tribe::inca );
    square   = { .x = 0, .y = 2 };
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "two dwellings meet" ) {
    W.add_dwelling( { .x = 1, .y = 0 }, e_tribe::aztec );
    W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    W.add_dwelling( { .x = 4, .y = 4 }, e_tribe::inca );
    square   = { .x = 0, .y = 1 };
    expected = {
        MeetTribe{
            .is_first      = true,
            .tribe         = e_tribe::aztec,
            .num_dwellings = 1,
            .land_awarded  = {},
        },
        MeetTribe{
            .is_first      = false,
            .tribe         = e_tribe::inca,
            .num_dwellings = 2,
            .land_awarded  = {},
        },
    };
    REQUIRE( f() == expected );
  }

  SECTION( "two same tribe meet once" ) {
    W.add_dwelling( { .x = 1, .y = 0 }, e_tribe::inca );
    W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    W.add_dwelling( { .x = 4, .y = 4 }, e_tribe::inca );
    square   = { .x = 0, .y = 1 };
    expected = {
        MeetTribe{
            .is_first      = true,
            .tribe         = e_tribe::inca,
            .num_dwellings = 3,
            .land_awarded  = {},
        },
    };
    REQUIRE( f() == expected );
  }

  SECTION( "land owned no colonies" ) {
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 2, .y = 2 }, e_tribe::inca );
    W.natives().owned_land()[{ .x = 1, .y = 1 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 2, .y = 1 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 3, .y = 1 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 1, .y = 2 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 2, .y = 2 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 3, .y = 2 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 1, .y = 3 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 2, .y = 3 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 3, .y = 3 }] = dwelling.id;
    square   = { .x = 3, .y = 3 };
    expected = {
        MeetTribe{
            .is_first      = true,
            .tribe         = e_tribe::inca,
            .num_dwellings = 1,
            .land_awarded  = {},
        },
    };
    REQUIRE( f() == expected );
  }

  SECTION( "land owned with colonies" ) {
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 2, .y = 2 }, e_tribe::inca );
    W.natives().owned_land()[{ .x = 1, .y = 1 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 2, .y = 1 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 3, .y = 1 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 1, .y = 2 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 2, .y = 2 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 3, .y = 2 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 1, .y = 3 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 2, .y = 3 }] = dwelling.id;
    W.natives().owned_land()[{ .x = 3, .y = 3 }] = dwelling.id;
    Colony& colony = W.add_colony( { .x = 1, .y = 3 } );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::food );
    square   = { .x = 3, .y = 3 };
    expected = {
        MeetTribe{ .is_first      = true,
                   .tribe         = e_tribe::inca,
                   .num_dwellings = 1,
                   .land_awarded  = { { .x = 1, .y = 3 },
                                      { .x = 2, .y = 3 } } },
    };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[meet-natives] perform_meet_tribe" ) {
  World                    W;
  MeetTribe                meet_tribe;
  e_declare_war_on_natives declare_war = {};
  e_tribe const            tribe       = e_tribe::cherokee;
  e_nation const           nation      = W.default_nation();

  auto f = [&] {
    perform_meet_tribe( W.ss(), W.default_player(), meet_tribe,
                        declare_war );
  };

  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, tribe );

  REQUIRE( !W.natives()
                .tribe_for( tribe )
                .relationship[nation]
                .has_value() );

  meet_tribe                     = { .is_first      = false,
                                     .tribe         = tribe,
                                     .num_dwellings = 3,
                                     .land_awarded  = { { .x = 1, .y = 0 },
                                                        { .x = 2, .y = 0 } } };
  auto& owned_land               = W.natives().owned_land();
  owned_land[{ .x = 1, .y = 0 }] = dwelling.id;
  owned_land[{ .x = 2, .y = 0 }] = dwelling.id;
  owned_land[{ .x = 2, .y = 1 }] = dwelling.id;

  SECTION( "no declare war" ) {
    declare_war = e_declare_war_on_natives::no;
    f();
    REQUIRE(
        W.natives().tribe_for( tribe ).relationship[nation] ==
        TribeRelationship{ .at_war       = false,
                           .tribal_alarm = 0 } );
    REQUIRE( !owned_land.contains( { .x = 1, .y = 0 } ) );
    REQUIRE( !owned_land.contains( { .x = 2, .y = 0 } ) );
    REQUIRE( owned_land.contains( { .x = 2, .y = 1 } ) );
    REQUIRE( owned_land[{ .x = 2, .y = 1 }] == dwelling.id );
  }

  SECTION( "declare war" ) {
    declare_war = e_declare_war_on_natives::yes;
    f();
    REQUIRE(
        W.natives().tribe_for( tribe ).relationship[nation] ==
        TribeRelationship{ .at_war = true, .tribal_alarm = 0 } );
  }
}

TEST_CASE( "[meet-natives] perform_meet_tribe_ui_sequence" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
