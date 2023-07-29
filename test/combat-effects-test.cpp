/****************************************************************
**combat-effects-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-07-22.
*
* Description: Unit tests for the combat-effects module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/combat-effects.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/ieuro-mind.hpp"

// Revolution Now
#include "src/damaged.rds.hpp"
#include "src/icombat.rds.hpp"
#include "src/society.rds.hpp"
#include "src/unit-mgr.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/tribe.rds.hpp"
#include "src/ss/units.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

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
        _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[combat-effects] euro_unit_combat_effects_msg" ) {
  World W;

  UnitId                unit_id = {};
  EuroUnitCombatOutcome outcome;
  CombatEffectsMessage  expected;

  auto f = [&] {
    return euro_unit_combat_effects_msg(
        W.units().unit_for( unit_id ), outcome );
  };

  SECTION( "no_change" ) {
    unit_id = W.add_unit_on_map( e_unit_type::free_colonist,
                                 { .x = 1, .y = 1 } )
                  .id();
    outcome  = EuroUnitCombatOutcome::no_change{};
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "destroyed" ) {
    unit_id = W.add_unit_on_map( e_unit_type::scout,
                                 { .x = 1, .y = 1 } )
                  .id();
    outcome  = EuroUnitCombatOutcome::destroyed{};
    expected = {
        .for_owner = {
            "Dutch [Scout] has been lost in battle!" } };
    REQUIRE( f() == expected );
  }

  SECTION( "captured" ) {
    unit_id = W.add_unit_on_map( e_unit_type::free_colonist,
                                 { .x = 1, .y = 1 } )
                  .id();
    outcome = EuroUnitCombatOutcome::captured{
        .new_nation = e_nation::french,
        .new_coord  = { .x = 0, .y = 1 } };
    expected = { .for_both = { "Dutch [Free Colonist] captured "
                               "by the [French]!" } };
    REQUIRE( f() == expected );
  }

  SECTION( "captured_and_demoted (veteran_colonist)" ) {
    unit_id = W.add_unit_on_map( e_unit_type::veteran_colonist,
                                 { .x = 1, .y = 1 } )
                  .id();
    outcome = EuroUnitCombatOutcome::captured_and_demoted{
        .to         = e_unit_type::free_colonist,
        .new_nation = e_nation::french,
        .new_coord  = { .x = 0, .y = 1 } };
    expected = {
        .for_other = { "Veteran status lost upon capture!" },
        .for_both = { "Dutch [Veteran Colonist] captured by the "
                      "[French]!" } };
    REQUIRE( f() == expected );
  }

  // This case doesn't really happen in the game, but we handle
  // it anyway.
  SECTION( "captured_and_demoted (other unit)" ) {
    unit_id = W.add_unit_on_map( e_unit_type::hardy_colonist,
                                 { .x = 1, .y = 1 } )
                  .id();
    outcome = EuroUnitCombatOutcome::captured_and_demoted{
        .to         = e_unit_type::free_colonist,
        .new_nation = e_nation::french,
        .new_coord  = { .x = 0, .y = 1 } };
    expected = { .for_other = { "Unit demoted upon capture!" },
                 .for_both = { "Dutch [Hardy Colonist] captured "
                               "by the [French]!" } };
    REQUIRE( f() == expected );
  }

  SECTION( "demoted" ) {
    unit_id = W.add_unit_on_map( e_unit_type::veteran_soldier,
                                 { .x = 1, .y = 1 } )
                  .id();
    outcome = EuroUnitCombatOutcome::demoted{
        .to = e_unit_type::veteran_colonist };
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "promoted" ) {
    unit_id = W.add_unit_on_map( e_unit_type::soldier,
                                 { .x = 1, .y = 1 } )
                  .id();
    outcome = EuroUnitCombatOutcome::promoted{
        .to = e_unit_type::veteran_soldier };
    expected = { .for_owner = {
                     "Unit promoted for victory in combat!" } };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[combat-effects] native_unit_combat_effects_msg" ) {
  World W;

  NativeUnitId            unit_id = {};
  NativeUnitCombatOutcome outcome;
  CombatEffectsMessage    expected;

  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 0 }, e_tribe::cherokee );

  auto f = [&] {
    return native_unit_combat_effects_msg(
        W.ss(), W.units().unit_for( unit_id ), outcome );
  };

  SECTION( "no_change" ) {
    unit_id = W.add_native_unit_on_map(
                   e_native_unit_type::brave, { .x = 1, .y = 1 },
                   dwelling.id )
                  .id;
    outcome  = NativeUnitCombatOutcome::no_change{};
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "destroyed" ) {
    unit_id = W.add_native_unit_on_map(
                   e_native_unit_type::brave, { .x = 1, .y = 1 },
                   dwelling.id )
                  .id;
    outcome = NativeUnitCombatOutcome::destroyed{
        .tribe_retains_horses  = true,
        .tribe_retains_muskets = true };
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "promoted, brave --> armed_brave" ) {
    unit_id = W.add_native_unit_on_map(
                   e_native_unit_type::brave, { .x = 1, .y = 1 },
                   dwelling.id )
                  .id;
    outcome = NativeUnitCombatOutcome::promoted{
        .to = e_native_unit_type::armed_brave };
    expected = {
        .for_both = { "[Cherokee] Brave has acquired [muskets] "
                      "upon victory in combat!" } };
    REQUIRE( f() == expected );
  }

  SECTION( "promoted, brave --> mounted_brave" ) {
    unit_id = W.add_native_unit_on_map(
                   e_native_unit_type::brave, { .x = 1, .y = 1 },
                   dwelling.id )
                  .id;
    outcome = NativeUnitCombatOutcome::promoted{
        .to = e_native_unit_type::mounted_brave };
    expected = {
        .for_both = { "[Cherokee] Brave has acquired [horses] "
                      "upon victory in combat!" } };
    REQUIRE( f() == expected );
  }

  SECTION( "promoted, mounted_brave --> mounted_warrior" ) {
    unit_id = W.add_native_unit_on_map(
                   e_native_unit_type::mounted_brave,
                   { .x = 1, .y = 1 }, dwelling.id )
                  .id;
    outcome = NativeUnitCombatOutcome::promoted{
        .to = e_native_unit_type::mounted_warrior };
    expected = {
        .for_both = { "[Cherokee] Mounted Brave has acquired "
                      "[muskets] upon victory in combat!" } };
    REQUIRE( f() == expected );
  }

  SECTION( "promoted, armed_brave --> mounted_warrior" ) {
    unit_id = W.add_native_unit_on_map(
                   e_native_unit_type::armed_brave,
                   { .x = 1, .y = 1 }, dwelling.id )
                  .id;
    outcome = NativeUnitCombatOutcome::promoted{
        .to = e_native_unit_type::mounted_warrior };
    expected = {
        .for_both = { "[Cherokee] Armed Brave has acquired "
                      "[horses] upon victory in combat!" } };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[combat-effects] naval_unit_combat_effects_msg" ) {
  World W;

  UnitId                     unit_id          = {};
  UnitId                     opponent_unit_id = {};
  EuroNavalUnitCombatOutcome outcome;
  CombatEffectsMessage       expected;

  auto f = [&] {
    return naval_unit_combat_effects_msg(
        W.ss(), W.units().unit_for( unit_id ),
        W.units().unit_for( opponent_unit_id ), outcome );
  };

  SECTION( "no_change" ) {
    unit_id =
        W.add_unit_on_map( e_unit_type::caravel,
                           { .x = 0, .y = 2 }, e_nation::dutch )
            .id();
    opponent_unit_id =
        W.add_unit_on_map( e_unit_type::privateer,
                           { .x = 0, .y = 3 }, e_nation::french )
            .id();
    outcome  = EuroNavalUnitCombatOutcome::no_change{};
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "moved" ) {
    unit_id =
        W.add_unit_on_map( e_unit_type::privateer,
                           { .x = 0, .y = 3 }, e_nation::dutch )
            .id();
    opponent_unit_id =
        W.add_unit_on_map( e_unit_type::caravel,
                           { .x = 0, .y = 2 }, e_nation::french )
            .id();
    outcome = EuroNavalUnitCombatOutcome::moved{
        .to = { .x = 0, .y = 2 } };
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "damaged, sent to harbor" ) {
    unit_id =
        W.add_unit_on_map( e_unit_type::privateer,
                           { .x = 0, .y = 3 }, e_nation::dutch )
            .id();
    opponent_unit_id =
        W.add_unit_on_map( e_unit_type::caravel,
                           { .x = 0, .y = 2 }, e_nation::french )
            .id();
    outcome = EuroNavalUnitCombatOutcome::damaged{
        .port = ShipRepairPort::european_harbor{} };
    expected = {
        .for_both = { "Dutch [Privateer] damaged in battle! "
                      "Ship sent to [Amsterdam] for repair." } };
    REQUIRE( f() == expected );
  }

  SECTION( "damaged, sent to colony" ) {
    Colony& colony =
        W.add_colony( { .x = 1, .y = 0 }, e_nation::dutch );
    colony.name = "Billooboo";
    unit_id =
        W.add_unit_on_map( e_unit_type::privateer,
                           { .x = 0, .y = 3 }, e_nation::dutch )
            .id();
    opponent_unit_id =
        W.add_unit_on_map( e_unit_type::caravel,
                           { .x = 0, .y = 2 }, e_nation::french )
            .id();
    outcome = EuroNavalUnitCombatOutcome::damaged{
        .port = ShipRepairPort::colony{ .id = colony.id } };
    expected = {
        .for_both = { "Dutch [Privateer] damaged in battle! "
                      "Ship sent to [Billooboo] for repair." } };
    REQUIRE( f() == expected );
  }

  SECTION( "damaged, sent to colony, one unit lost" ) {
    Colony& colony =
        W.add_colony( { .x = 1, .y = 0 }, e_nation::dutch );
    colony.name = "Billooboo";
    unit_id =
        W.add_unit_on_map( e_unit_type::privateer,
                           { .x = 0, .y = 3 }, e_nation::dutch )
            .id();
    W.add_unit_in_cargo( e_unit_type::free_colonist, unit_id );
    opponent_unit_id =
        W.add_unit_on_map( e_unit_type::caravel,
                           { .x = 0, .y = 2 }, e_nation::french )
            .id();
    outcome = EuroNavalUnitCombatOutcome::damaged{
        .port = ShipRepairPort::colony{ .id = colony.id } };
    expected = {
        .for_owner = { "[One] unit onboard has been lost." },
        .for_both  = { "Dutch [Privateer] damaged in battle! "
                        "Ship sent to [Billooboo] for repair." } };
    REQUIRE( f() == expected );
  }

  SECTION( "sunk" ) {
    unit_id =
        W.add_unit_on_map( e_unit_type::frigate,
                           { .x = 0, .y = 3 }, e_nation::dutch )
            .id();
    opponent_unit_id =
        W.add_unit_on_map( e_unit_type::merchantman,
                           { .x = 0, .y = 2 }, e_nation::french )
            .id();
    outcome  = EuroNavalUnitCombatOutcome::sunk{};
    expected = {
        .for_both = {
            "Dutch [Frigate] sunk by [French] Merchantman." } };
    REQUIRE( f() == expected );
  }

  SECTION( "sunk, three units on board lost" ) {
    unit_id =
        W.add_unit_on_map( e_unit_type::frigate,
                           { .x = 0, .y = 3 }, e_nation::dutch )
            .id();
    W.add_unit_in_cargo( e_unit_type::free_colonist, unit_id );
    W.add_unit_in_cargo( e_unit_type::free_colonist, unit_id );
    W.add_unit_in_cargo( e_unit_type::free_colonist, unit_id );
    opponent_unit_id =
        W.add_unit_on_map( e_unit_type::merchantman,
                           { .x = 0, .y = 2 }, e_nation::french )
            .id();
    outcome  = EuroNavalUnitCombatOutcome::sunk{};
    expected = {
        .for_owner = { "[Three] units onboard have been lost." },
        .for_both  = {
            "Dutch [Frigate] sunk by [French] Merchantman." } };
    REQUIRE( f() == expected );
  }
}

TEST_CASE(
    "[combat-effects] perform_euro_unit_combat_effects" ) {
  World W;

  UnitId                unit_id = {};
  EuroUnitCombatOutcome outcome;

  auto f = [&] {
    perform_euro_unit_combat_effects(
        W.ss(), W.ts(), W.units().unit_for( unit_id ), outcome );
  };

  SECTION( "no_change" ) {
    Unit& unit = W.add_unit_on_map( e_unit_type::free_colonist,
                                    { .x = 1, .y = 1 } );
    unit.sentry();
    unit_id = unit.id();
    outcome = EuroUnitCombatOutcome::no_change{};
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type() == e_unit_type::free_colonist );
    REQUIRE( unit.nation() == e_nation::dutch );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
    REQUIRE( unit.movement_points() == 1 );
    REQUIRE( unit.orders() == unit_orders::sentry{} );
  }

  SECTION( "destroyed" ) {
    unit_id = W.add_unit_on_map( e_unit_type::scout,
                                 { .x = 1, .y = 1 } )
                  .id();
    outcome = EuroUnitCombatOutcome::destroyed{};
    f();
    REQUIRE( !W.units().exists( unit_id ) );
    // !! No further tests since unit does not exist.
  }

  SECTION( "captured" ) {
    Unit& unit = W.add_unit_on_map( e_unit_type::free_colonist,
                                    { .x = 1, .y = 1 } );
    unit.sentry();
    unit_id = unit.id();
    outcome = EuroUnitCombatOutcome::captured{
        .new_nation = e_nation::french,
        .new_coord  = { .x = 0, .y = 1 } };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type() == e_unit_type::free_colonist );
    REQUIRE( unit.nation() == e_nation::french );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
    REQUIRE( unit.movement_points() == 0 );
    REQUIRE( unit.orders() == unit_orders::none{} );
  }

  SECTION( "captured_and_demoted (veteran_colonist)" ) {
    Unit& unit = W.add_unit_on_map(
        e_unit_type::veteran_colonist, { .x = 1, .y = 1 } );
    unit.fortify();
    unit_id = unit.id();
    outcome = EuroUnitCombatOutcome::captured_and_demoted{
        .to         = e_unit_type::free_colonist,
        .new_nation = e_nation::french,
        .new_coord  = { .x = 0, .y = 1 } };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type() == e_unit_type::free_colonist );
    REQUIRE( unit.nation() == e_nation::french );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
    REQUIRE( unit.movement_points() == 0 );
    REQUIRE( unit.orders() == unit_orders::none{} );
  }

  // This case doesn't really happen in the game, but we handle
  // it anyway.
  SECTION( "captured_and_demoted (other unit)" ) {
    Unit& unit = W.add_unit_on_map( e_unit_type::hardy_colonist,
                                    { .x = 1, .y = 1 } );
    unit.fortify();
    unit_id = unit.id();
    outcome = EuroUnitCombatOutcome::captured_and_demoted{
        .to         = e_unit_type::free_colonist,
        .new_nation = e_nation::french,
        .new_coord  = { .x = 0, .y = 1 } };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type() == e_unit_type::free_colonist );
    REQUIRE( unit.nation() == e_nation::french );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
    REQUIRE( unit.movement_points() == 0 );
    REQUIRE( unit.orders() == unit_orders::none{} );
  }

  SECTION( "demoted" ) {
    Unit& unit = W.add_unit_on_map( e_unit_type::veteran_soldier,
                                    { .x = 1, .y = 1 } );
    unit.fortify();
    unit.new_turn( W.default_player() );
    unit_id = unit.id();
    outcome = EuroUnitCombatOutcome::demoted{
        .to = e_unit_type::veteran_colonist };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type() == e_unit_type::veteran_colonist );
    REQUIRE( unit.nation() == e_nation::dutch );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
    REQUIRE( unit.movement_points() == 1 );
    REQUIRE( unit.orders() == unit_orders::fortified{} );
  }

  SECTION( "promoted" ) {
    Unit& unit = W.add_unit_on_map( e_unit_type::soldier,
                                    { .x = 1, .y = 1 } );
    unit.fortify();
    unit.new_turn( W.default_player() );
    unit_id = unit.id();
    outcome = EuroUnitCombatOutcome::promoted{
        .to = e_unit_type::veteran_soldier };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type() == e_unit_type::veteran_soldier );
    REQUIRE( unit.nation() == e_nation::dutch );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
    REQUIRE( unit.movement_points() == 1 );
    REQUIRE( unit.orders() == unit_orders::fortified{} );
  }
}

TEST_CASE(
    "[combat-effects] perform_native_unit_combat_effects" ) {
  World W;

  NativeUnitId            unit_id = {};
  NativeUnitCombatOutcome outcome;

  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 0 }, e_tribe::cherokee );

  auto f = [&] {
    perform_native_unit_combat_effects(
        W.ss(), W.units().unit_for( unit_id ), outcome );
  };

  SECTION( "no_change" ) {
    NativeUnit const& unit = W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 1, .y = 1 },
        dwelling.id );
    unit_id = unit.id;
    outcome = NativeUnitCombatOutcome::no_change{};
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type == e_native_unit_type::brave );
    REQUIRE( tribe_for_unit( W.ss(), unit ) ==
             e_tribe::cherokee );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
    REQUIRE( unit.movement_points == 1 );
    REQUIRE( W.tribe( e_tribe::cherokee ).horses == 0 );
    REQUIRE( W.tribe( e_tribe::cherokee ).muskets == 0 );
  }

  SECTION( "destroyed" ) {
    NativeUnit const& unit = W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 1, .y = 1 },
        dwelling.id );
    unit_id = unit.id;
    outcome = NativeUnitCombatOutcome::destroyed{
        .tribe_retains_horses  = true,
        .tribe_retains_muskets = true };
    f();
    REQUIRE( !W.units().exists( unit_id ) );
    REQUIRE( W.tribe( e_tribe::cherokee ).horses == 50 );
    REQUIRE( W.tribe( e_tribe::cherokee ).muskets == 50 );
  }

  SECTION( "promoted, brave --> armed_brave" ) {
    NativeUnit const& unit = W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 1, .y = 1 },
        dwelling.id );
    unit_id = unit.id;
    outcome = NativeUnitCombatOutcome::promoted{
        .to = e_native_unit_type::armed_brave };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type == e_native_unit_type::armed_brave );
    REQUIRE( tribe_for_unit( W.ss(), unit ) ==
             e_tribe::cherokee );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
    REQUIRE( unit.movement_points == 1 );
    REQUIRE( W.tribe( e_tribe::cherokee ).horses == 0 );
    REQUIRE( W.tribe( e_tribe::cherokee ).muskets == 0 );
  }

  SECTION( "promoted, brave --> mounted_brave" ) {
    NativeUnit const& unit = W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 1, .y = 1 },
        dwelling.id );
    unit_id = unit.id;
    outcome = NativeUnitCombatOutcome::promoted{
        .to = e_native_unit_type::mounted_brave };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type == e_native_unit_type::mounted_brave );
    REQUIRE( tribe_for_unit( W.ss(), unit ) ==
             e_tribe::cherokee );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
    // The unit should keep its current movement points instead
    // of having them increase from 1 to 4.
    REQUIRE( unit.movement_points == 1 );
    REQUIRE( W.tribe( e_tribe::cherokee ).horses == 0 );
    REQUIRE( W.tribe( e_tribe::cherokee ).muskets == 0 );
  }

  SECTION( "promoted, mounted_brave --> mounted_warrior" ) {
    NativeUnit const& unit = W.add_native_unit_on_map(
        e_native_unit_type::mounted_brave, { .x = 1, .y = 1 },
        dwelling.id );
    unit_id = unit.id;
    outcome = NativeUnitCombatOutcome::promoted{
        .to = e_native_unit_type::mounted_warrior };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type == e_native_unit_type::mounted_warrior );
    REQUIRE( tribe_for_unit( W.ss(), unit ) ==
             e_tribe::cherokee );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
    REQUIRE( unit.movement_points == 4 );
    REQUIRE( W.tribe( e_tribe::cherokee ).horses == 0 );
    REQUIRE( W.tribe( e_tribe::cherokee ).muskets == 0 );
  }

  SECTION( "promoted, armed_brave --> mounted_warrior" ) {
    NativeUnit const& unit = W.add_native_unit_on_map(
        e_native_unit_type::armed_brave, { .x = 1, .y = 1 },
        dwelling.id );
    unit_id = unit.id;
    outcome = NativeUnitCombatOutcome::promoted{
        .to = e_native_unit_type::mounted_warrior };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type == e_native_unit_type::mounted_warrior );
    REQUIRE( tribe_for_unit( W.ss(), unit ) ==
             e_tribe::cherokee );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
    // The unit should keep its current movement points instead
    // of having them increase from 1 to 4.
    REQUIRE( unit.movement_points == 1 );
    REQUIRE( W.tribe( e_tribe::cherokee ).horses == 0 );
    REQUIRE( W.tribe( e_tribe::cherokee ).muskets == 0 );
  }
}

TEST_CASE(
    "[combat-effects] perform_naval_unit_combat_effects" ) {
  World W;

  UnitId                     unit_id          = {};
  UnitId                     opponent_unit_id = {};
  EuroNavalUnitCombatOutcome outcome;

  auto f = [&] {
    perform_naval_unit_combat_effects(
        W.ss(), W.ts(), W.units().unit_for( unit_id ),
        opponent_unit_id, outcome );
  };

  SECTION( "no_change" ) {
    Unit& unit =
        W.add_unit_on_map( e_unit_type::caravel,
                           { .x = 0, .y = 2 }, e_nation::dutch );
    unit_id = unit.id();
    UnitId const onboard_id =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             unit_id )
            .id();
    Unit const& opponent_unit = W.add_unit_on_map(
        e_unit_type::privateer, { .x = 0, .y = 3 },
        e_nation::french );
    // Need to do this after adding the other unit otherwise the
    // sentry'd unit will get unsentry'd.
    unit.sentry();
    opponent_unit_id = opponent_unit.id();
    outcome          = EuroNavalUnitCombatOutcome::no_change{};
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type() == e_unit_type::caravel );
    REQUIRE( unit.nation() == e_nation::dutch );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 2 } );
    REQUIRE( unit.movement_points() == 4 );
    REQUIRE( unit.orders() == unit_orders::sentry{} );
    REQUIRE( W.units().exists( onboard_id ) );
    REQUIRE( unit.cargo().units().size() == 1 );
  }

  SECTION( "moved" ) {
    Unit& unit =
        W.add_unit_on_map( e_unit_type::privateer,
                           { .x = 0, .y = 3 }, e_nation::dutch );
    unit_id = unit.id();
    UnitId const onboard_id =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             unit_id )
            .id();
    Unit const& opponent_unit = W.add_unit_on_map(
        e_unit_type::caravel, { .x = 0, .y = 2 },
        e_nation::french );
    opponent_unit_id = opponent_unit.id();
    outcome          = EuroNavalUnitCombatOutcome::moved{
                 .to = { .x = 0, .y = 2 } };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type() == e_unit_type::privateer );
    REQUIRE( unit.nation() == e_nation::dutch );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 2 } );
    REQUIRE( unit.movement_points() == 8 );
    REQUIRE( unit.orders() == unit_orders::none{} );
    REQUIRE( W.units().exists( onboard_id ) );
    REQUIRE( unit.cargo().units().size() == 1 );
  }

  SECTION( "damaged, sent to harbor" ) {
    Unit& unit =
        W.add_unit_on_map( e_unit_type::privateer,
                           { .x = 0, .y = 3 }, e_nation::dutch );
    unit_id = unit.id();
    UnitId const onboard_id =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             unit_id )
            .id();
    Unit const& opponent_unit = W.add_unit_on_map(
        e_unit_type::caravel, { .x = 0, .y = 2 },
        e_nation::french );
    opponent_unit_id = opponent_unit.id();
    outcome          = EuroNavalUnitCombatOutcome::damaged{
                 .port = ShipRepairPort::european_harbor{} };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type() == e_unit_type::privateer );
    REQUIRE( unit.nation() == e_nation::dutch );
    REQUIRE( as_const( W.units() ).ownership_of( unit_id ) ==
             UnitOwnership::harbor{
                 .st = UnitHarborViewState{
                     .port_status = PortStatus::in_port{},
                     .sailed_from = nothing } } );
    REQUIRE( unit.movement_points() == 8 );
    REQUIRE( unit.orders() ==
             unit_orders::damaged{ .turns_until_repair = 8 } );
    REQUIRE( !W.units().exists( onboard_id ) );
    REQUIRE( unit.cargo().units().size() == 0 );
  }

  SECTION( "damaged, sent to colony" ) {
    Colony& colony =
        W.add_colony( { .x = 1, .y = 0 }, e_nation::dutch );
    colony.name = "Billooboo";
    Unit& unit =
        W.add_unit_on_map( e_unit_type::privateer,
                           { .x = 0, .y = 3 }, e_nation::dutch );
    unit_id                   = unit.id();
    Unit const& opponent_unit = W.add_unit_on_map(
        e_unit_type::caravel, { .x = 0, .y = 2 },
        e_nation::french );
    opponent_unit_id = opponent_unit.id();
    outcome          = EuroNavalUnitCombatOutcome::damaged{
                 .port = ShipRepairPort::colony{ .id = colony.id } };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type() == e_unit_type::privateer );
    REQUIRE( unit.nation() == e_nation::dutch );
    REQUIRE( as_const( W.units() ).ownership_of( unit_id ) ==
             UnitOwnership::world{ .coord = colony.location } );
    REQUIRE( unit.movement_points() == 8 );
    REQUIRE( unit.orders() ==
             unit_orders::damaged{ .turns_until_repair = 3 } );
    REQUIRE( unit.cargo().units().size() == 0 );
  }

  SECTION( "damaged, sent to colony, one unit lost" ) {
    Colony& colony =
        W.add_colony( { .x = 1, .y = 0 }, e_nation::dutch );
    colony.name = "Billooboo";
    Unit& unit =
        W.add_unit_on_map( e_unit_type::privateer,
                           { .x = 0, .y = 3 }, e_nation::dutch );
    unit_id = unit.id();
    UnitId const onboard_id =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             unit_id )
            .id();
    Unit const& opponent_unit = W.add_unit_on_map(
        e_unit_type::caravel, { .x = 0, .y = 2 },
        e_nation::french );
    opponent_unit_id = opponent_unit.id();
    outcome          = EuroNavalUnitCombatOutcome::damaged{
                 .port = ShipRepairPort::colony{ .id = colony.id } };
    f();
    REQUIRE( W.units().exists( unit_id ) );
    REQUIRE( unit.type() == e_unit_type::privateer );
    REQUIRE( unit.nation() == e_nation::dutch );
    REQUIRE( as_const( W.units() ).ownership_of( unit_id ) ==
             UnitOwnership::world{ .coord = colony.location } );
    REQUIRE( unit.movement_points() == 8 );
    REQUIRE( unit.orders() ==
             unit_orders::damaged{ .turns_until_repair = 3 } );
    REQUIRE( !W.units().exists( onboard_id ) );
    REQUIRE( unit.cargo().units().size() == 0 );
  }

  SECTION( "sunk" ) {
    Unit& unit =
        W.add_unit_on_map( e_unit_type::frigate,
                           { .x = 0, .y = 3 }, e_nation::dutch );
    unit_id                   = unit.id();
    Unit const& opponent_unit = W.add_unit_on_map(
        e_unit_type::merchantman, { .x = 0, .y = 2 },
        e_nation::french );
    opponent_unit_id = opponent_unit.id();
    outcome          = EuroNavalUnitCombatOutcome::sunk{};
    f();
    REQUIRE( !W.units().exists( unit_id ) );
  }

  SECTION( "sunk, three units on board lost" ) {
    Unit& unit =
        W.add_unit_on_map( e_unit_type::frigate,
                           { .x = 0, .y = 3 }, e_nation::dutch );
    unit_id = unit.id();
    UnitId const onboard_id1 =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             unit_id )
            .id();
    UnitId const onboard_id2 =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             unit_id )
            .id();
    UnitId const onboard_id3 =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             unit_id )
            .id();
    Unit const& opponent_unit = W.add_unit_on_map(
        e_unit_type::merchantman, { .x = 0, .y = 2 },
        e_nation::french );
    opponent_unit_id = opponent_unit.id();
    outcome          = EuroNavalUnitCombatOutcome::sunk{};
    f();
    REQUIRE( !W.units().exists( unit_id ) );
    REQUIRE( !W.units().exists( onboard_id1 ) );
    REQUIRE( !W.units().exists( onboard_id2 ) );
    REQUIRE( !W.units().exists( onboard_id3 ) );
  }
}

TEST_CASE(
    "[combat-effects] show_combat_effects_messages_euro_euro" ) {
  World          W;
  MockIEuroMind& attacker_mind = W.euro_mind( e_nation::dutch );
  MockIEuroMind& defender_mind = W.euro_mind( e_nation::french );
  EuroCombatEffectsMessage attacker{ .mind = attacker_mind };
  EuroCombatEffectsMessage defender{ .mind = defender_mind };

  auto f = [&] {
    wait<> const w = show_combat_effects_messages_euro_euro(
        attacker, defender );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
  };

  attacker.msg = CombatEffectsMessage{
      .for_owner = { "111 111", "222 222" },
      .for_other = { "333 333", "444 444" },
      .for_both  = { "555 555", "666 666" },
  };

  defender.msg = CombatEffectsMessage{
      .for_owner = { "777 777", "888 888" },
      .for_other = { "999 999", "aaa aaa" },
      .for_both  = { "bbb bbb", "ccc ccc" },
  };

  attacker_mind
      .EXPECT__message_box(
          "555 555 666 666 111 111 222 222 bbb bbb ccc ccc 999 "
          "999 aaa aaa" )
      .returns<monostate>();
  defender_mind
      .EXPECT__message_box(
          "bbb bbb ccc ccc 777 777 888 888 555 555 666 666 333 "
          "333 444 444" )
      .returns<monostate>();
  f();
}

TEST_CASE(
    "[combat-effects] "
    "show_combat_effects_messages_euro_attacker_only" ) {
  World          W;
  MockIEuroMind& attacker_mind = W.euro_mind( e_nation::dutch );
  EuroCombatEffectsMessage attacker{ .mind = attacker_mind };

  auto f = [&] {
    wait<> const w =
        show_combat_effects_messages_euro_attacker_only(
            attacker );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
  };

  attacker.msg = CombatEffectsMessage{
      .for_owner = { "111 111", "222 222" },
      .for_other = { "333 333", "444 444" },
      .for_both  = { "555 555", "666 666" },
  };

  attacker_mind
      .EXPECT__message_box( "555 555 666 666 111 111 222 222" )
      .returns<monostate>();
  f();
}

TEST_CASE(
    "[combat-effects] "
    "show_combat_effects_messages_euro_native" ) {
  World          W;
  MockIEuroMind& euro_mind = W.euro_mind( e_nation::dutch );
  EuroCombatEffectsMessage   euro{ .mind = euro_mind };
  NativeCombatEffectsMessage native;

  auto f = [&] {
    wait<> const w =
        show_combat_effects_messages_euro_native( euro, native );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
  };

  euro.msg = CombatEffectsMessage{
      .for_owner = { "111 111", "222 222" },
      .for_other = { "333 333", "444 444" },
      .for_both  = { "555 555", "666 666" },
  };
  native.msg = CombatEffectsMessage{
      .for_owner = { "777 777", "888 888" },
      .for_other = { "999 999", "aaa aaa" },
      .for_both  = { "bbb bbb", "ccc ccc" },
  };

  euro_mind
      .EXPECT__message_box(
          "555 555 666 666 111 111 222 222 bbb bbb ccc ccc 999 "
          "999 aaa aaa" )
      .returns<monostate>();
  f();
}

} // namespace
} // namespace rn
