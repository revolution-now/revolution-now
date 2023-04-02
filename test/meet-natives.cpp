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

// Revolution Now
#include "src/native-owned.hpp"

// ss
#include "ss/natives.hpp"
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
            .nation        = e_nation::english,
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
    inca.relationship[W.default_nation()].encountered = true;
    expected                                          = {};
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
            .nation        = e_nation::english,
            .tribe         = e_tribe::aztec,
            .num_dwellings = 1,
            .land_awarded  = {},
        },
        MeetTribe{
            .nation        = e_nation::english,
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
            .nation        = e_nation::english,
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
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 3 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 3 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 3 } );
    square   = { .x = 3, .y = 3 };
    expected = {
        MeetTribe{
            .nation        = e_nation::english,
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
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 3 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 3 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 3 } );
    Colony& colony = W.add_colony( { .x = 1, .y = 3 } );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::food );
    square   = { .x = 3, .y = 3 };
    expected = {
        MeetTribe{ .nation        = e_nation::english,
                   .tribe         = e_tribe::inca,
                   .num_dwellings = 1,
                   .land_awarded  = { { .x = 1, .y = 3 },
                                      { .x = 2, .y = 3 } } },
    };
    REQUIRE( f() == expected );
  }

  SECTION( "land owned with colonies and minuit" ) {
    W.default_player()
        .fathers.has[e_founding_father::peter_minuit] = true;
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 2, .y = 2 }, e_tribe::inca );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 1 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 2 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 1, .y = 3 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 2, .y = 3 } );
    W.natives().mark_land_owned( dwelling.id,
                                 { .x = 3, .y = 3 } );
    Colony& colony = W.add_colony( { .x = 1, .y = 3 } );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::food );
    square   = { .x = 3, .y = 3 };
    expected = {
        MeetTribe{ .nation        = e_nation::english,
                   .tribe         = e_tribe::inca,
                   .num_dwellings = 1,
                   .land_awarded  = {} },
    };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[meet-natives] perform_meet_tribe" ) {
  World                    W;
  Player const&            player = W.default_player();
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
                .encountered );

  meet_tribe = { .nation        = e_nation::dutch,
                 .tribe         = tribe,
                 .num_dwellings = 3,
                 .land_awarded  = { { .x = 1, .y = 0 },
                                    { .x = 2, .y = 0 } } };
  W.natives().mark_land_owned( dwelling.id, { .x = 1, .y = 0 } );
  W.natives().mark_land_owned( dwelling.id, { .x = 2, .y = 0 } );
  W.natives().mark_land_owned( dwelling.id, { .x = 2, .y = 1 } );

  SECTION( "no declare war" ) {
    declare_war = e_declare_war_on_natives::no;
    f();
    REQUIRE(
        W.natives().tribe_for( tribe ).relationship[nation] ==
        TribeRelationship{ .encountered  = true,
                           .at_war       = false,
                           .tribal_alarm = 10 } );
    REQUIRE( !is_land_native_owned( W.ss(), player,
                                    { .x = 1, .y = 0 } ) );
    REQUIRE( !is_land_native_owned( W.ss(), player,
                                    { .x = 2, .y = 0 } ) );
    REQUIRE( is_land_native_owned( W.ss(), player,
                                   { .x = 2, .y = 1 } ) );
    REQUIRE( is_land_native_owned( W.ss(), player,
                                   { .x = 2, .y = 1 } ) );
  }

  SECTION( "declare war" ) {
    declare_war = e_declare_war_on_natives::yes;
    f();
    REQUIRE(
        W.natives().tribe_for( tribe ).relationship[nation] ==
        TribeRelationship{ .encountered  = true,
                           .at_war       = true,
                           .tribal_alarm = 10 } );
  }

  // The perform_meet_tribe function may receive some owned land
  // squares occupied by colonies, since those are among the ones
  // given to the player upon meeting. This test makes sure that
  // it doesn't check fail in that case.
  SECTION( "with colony" ) {
    declare_war = e_declare_war_on_natives::no;
    W.add_colony( { .x = 1, .y = 0 } );
    f();
    REQUIRE(
        W.natives().tribe_for( tribe ).relationship[nation] ==
        TribeRelationship{ .encountered  = true,
                           .at_war       = false,
                           .tribal_alarm = 10 } );
    REQUIRE( !is_land_native_owned( W.ss(), player,
                                    { .x = 1, .y = 0 } ) );
    REQUIRE( !is_land_native_owned( W.ss(), player,
                                    { .x = 2, .y = 0 } ) );
    REQUIRE( is_land_native_owned( W.ss(), player,
                                   { .x = 2, .y = 1 } ) );
    REQUIRE( is_land_native_owned( W.ss(), player,
                                   { .x = 2, .y = 1 } ) );
  }
}

// This one tests that when the relationship object is created
// the tribal alarm is initialized with the minimum value.
TEST_CASE( "[meet-natives] perform_meet_tribe arawak" ) {
  World                    W;
  MeetTribe                meet_tribe;
  e_declare_war_on_natives declare_war = {};
  e_tribe const            tribe       = e_tribe::arawak;
  Tribe const&             tribe_obj   = W.add_tribe( tribe );
  e_nation const           nation      = W.default_nation();

  auto f = [&] {
    perform_meet_tribe( W.ss(), W.default_player(), meet_tribe,
                        declare_war );
  };

  meet_tribe = { .nation        = e_nation::dutch,
                 .tribe         = tribe,
                 .num_dwellings = 3 };
  f();
  REQUIRE( tribe_obj.relationship[nation] ==
           TribeRelationship{ .encountered  = true,
                              .at_war       = false,
                              .tribal_alarm = 30 } );
}

} // namespace
} // namespace rn
