/****************************************************************
**attack-handlers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-01.
*
* Description: Unit tests for the src/attack-handlers.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/attack-handlers.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/icombat.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/land-view-plane.hpp"

// Revolution Now
#include "src/orders.hpp"
#include "src/plane-stack.hpp"

// config
#include "config/unit-type.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/native-unit.rds.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/tribe.rds.hpp"
#include "src/ss/unit.hpp"
#include "src/ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;
using ::mock::matchers::StrContains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;

  static inline e_nation const kAttackingNation =
      e_nation::english;
  static inline e_nation const kDefendingNation =
      e_nation::french;

  static inline e_tribe const kNativeTribe = e_tribe::apache;

  World() : Base() {
    add_player( kAttackingNation );
    add_player( kDefendingNation );
    set_human_player( kAttackingNation );
    common_player_init( player( kAttackingNation ) );
    common_player_init( player( kDefendingNation ) );
    set_default_player( kAttackingNation );
    create_default_map();
    planes().back().land_view = &mock_land_view_plane_;
    Tribe& tribe              = add_tribe( kNativeTribe );
    tribe.relationship[kAttackingNation].emplace();
  }

  void common_player_init( Player& player ) const {
    // Suppress some woodcuts.
    player.woodcuts[e_woodcut::discovered_new_world] = true;
    player.new_world_name                            = "";
  }

  static inline Coord const kWaterAttack = { .x = 0, .y = 0 };
  static inline Coord const kWaterDefend = { .x = 1, .y = 0 };
  static inline Coord const kLandAttack  = { .x = 0, .y = 1 };
  static inline Coord const kLandDefend  = { .x = 1, .y = 1 };

  UnitId add_attacker( e_unit_type type ) {
    if( unit_attr( type ).ship )
      return add_unit_on_map( type, kWaterAttack,
                              kAttackingNation )
          .id();
    else
      return add_unit_on_map( type, kLandAttack,
                              kAttackingNation )
          .id();
  }

  UnitId add_defender( e_unit_type type ) {
    if( unit_attr( type ).ship )
      return add_unit_on_map( type, kWaterDefend,
                              kDefendingNation )
          .id();
    else
      return add_unit_on_map( type, kLandDefend,
                              kDefendingNation )
          .id();
  }

  NativeUnitId add_defender( e_native_unit_type type ) {
    DwellingId const dwelling_id =
        add_dwelling( { .x = 2, .y = 2 }, kNativeTribe ).id;
    return add_native_unit_on_map( type, kLandDefend,
                                   dwelling_id )
        .id;
  }

  auto add_pair( auto attacker_type, auto defender_type ) {
    auto attacker_id = add_attacker( attacker_type );
    auto defender_id = add_defender( defender_type );
    return pair{ attacker_id, defender_id };
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        // The upper-left 2x2 squares in this map are specially
        // setup so that we will automatically place attackers
        // and defenders on the following squares depending on
        // whether they are ships or non-ships:
        //
        //                  0               1
        //   0  AttackingShip   DefendingShip
        //   1  AttackingLand   DefendingLand
        //
        // This way, each attacker can attack either of the de-
        // fenders in order to test various possible scenarios.
        _, _, L, //
        L, L, L, //
        L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }

  OrdersHandler::RunResult run_handler(
      unique_ptr<OrdersHandler> handler ) {
    wait<OrdersHandler::RunResult> const w = handler->run();
    // Use check-fails so that we can get stack traces and know
    // which test case called us.
    CHECK( !w.exception() );
    CHECK( w.ready() );
    return *w;
  }

  void expect_msg_contains( string_view msg ) {
    EXPECT_CALL( gui(),
                 message_box( StrContains( string( msg ) ) ) )
        .returns<monostate>();
  }

  void expect_some_animation() {
    EXPECT_CALL( mock_land_view_plane_, animate( _ ) )
        .returns<monostate>();
  }

  void set_active_player( e_nation nation ) {
    set_human_player( nation );
    active_nation_ = nation;
  }

  MockLandViewPlane mock_land_view_plane_;

  // By convention this is the active player whose turn it would
  // currently be if this were a real game. We need to supply
  // this to the methods so they know which nation's units'
  // changes need to be flagged via message boxes. E.g., if a
  // non-human player's unit gets promoted we don't show a mes-
  // sage. Note that this can be changed as needed.
  e_nation active_nation_ = kAttackingNation;
};

/****************************************************************
** Test Cases
*****************************************************************/
// This test case tests failure modes that are common to most of
// the handlers.
#ifndef COMPILER_GCC
TEST_CASE( "[attack-handlers] common failure checks" ) {
  World                W;
  CombatEuroAttackEuro combat;

  auto expect_combat = [&] {
    EXPECT_CALL( W.combat(),
                 euro_attack_euro(
                     W.units().unit_for( combat.attacker.id ),
                     W.units().unit_for( combat.defender.id ) ) )
        .returns( combat );
  };

  auto f = [&] {
    return W.run_handler( attack_euro_land_handler(
        W.planes(), W.ss(), W.ts(), W.player( W.active_nation_ ),
        combat.attacker.id, combat.defender.id ) );
  };

  SECTION( "ships cannot attack land units" ) {
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::frigate, e_unit_type::free_colonist );
    W.expect_msg_contains( "Ships cannot attack land units" );
    f();
  }

  SECTION( "non-military units cannot attack" ) {
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::free_colonist, e_unit_type::free_colonist );
    W.expect_msg_contains( "unit cannot attack" );
    f();
  }

  SECTION( "cannot attack a land unit from a ship" ) {
    auto [ship_id, defender_id] = W.add_pair(
        e_unit_type::caravel, e_unit_type::free_colonist );
    combat.attacker.id =
        W.add_unit_in_cargo( e_unit_type::soldier, ship_id,
                             W.kAttackingNation )
            .id();
    combat.defender.id = defender_id;
    W.expect_msg_contains(
        "cannot attack a land unit from a ship" );
    f();
  }

  SECTION( "land unit cannot attack ship" ) {
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::caravel );
    W.expect_msg_contains( "Land units cannot attack ship" );
    f();
  }

  SECTION( "partial movement cancelled" ) {
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    W.units()
        .unit_for( combat.attacker.id )
        .consume_mv_points( MovementPoints::_1_3() );
    EXPECT_CALL( W.gui(), choice( _, _ ) )
        .returns<maybe<string>>( "no" );
    f();
  }

  SECTION( "partial movement proceed" ) {
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    W.units()
        .unit_for( combat.attacker.id )
        .consume_mv_points( MovementPoints::_1_3() );
    EXPECT_CALL( W.gui(), choice( _, _ ) )
        .returns<maybe<string>>( "yes" );
    expect_combat();
    W.expect_some_animation();
    f();
    // We're not concerned with the effects of the combat here,
    // that will be tested in other test cases.
  }
}
#endif

#ifndef COMPILER_GCC
TEST_CASE( "[attack-handlers] attack_euro_land_handler" ) {
  World                W;
  CombatEuroAttackEuro combat;

  auto expect_combat = [&] {
    EXPECT_CALL( W.combat(),
                 euro_attack_euro(
                     W.units().unit_for( combat.attacker.id ),
                     W.units().unit_for( combat.defender.id ) ) )
        .returns( combat );
  };

  auto f = [&] {
    return W.run_handler( attack_euro_land_handler(
        W.planes(), W.ss(), W.ts(), W.player( W.active_nation_ ),
        combat.attacker.id, combat.defender.id ) );
  };

  SECTION( "soldier->soldier, attacker loses" ) {
    combat = {
        .winner = e_combat_winner::defender,
        .attacker =
            { .outcome =
                  EuroUnitCombatOutcome::demoted{
                      .to = UnitType::create(
                          e_unit_type::free_colonist ) } },
        .defender = { .outcome =
                          EuroUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    f();
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( attacker.type() == e_unit_type::free_colonist );
    REQUIRE( defender.type() == e_unit_type::soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kLandDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
  }

  SECTION(
      "soldier->soldier, attacker loses, defender promoted" ) {
    combat = {
        .winner = e_combat_winner::defender,
        .attacker =
            { .outcome =
                  EuroUnitCombatOutcome::demoted{
                      .to = UnitType::create(
                          e_unit_type::free_colonist ) } },
        .defender = {
            .outcome = EuroUnitCombatOutcome::promoted{
                .to = UnitType::create(
                    e_unit_type::veteran_soldier ) } } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    f();
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( attacker.type() == e_unit_type::free_colonist );
    REQUIRE( defender.type() == e_unit_type::veteran_soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kLandDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
  }

  SECTION(
      "soldier->soldier, attacker wins with no promotion" ) {
    combat = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = { .outcome = EuroUnitCombatOutcome::demoted{
                          .to = UnitType::create(
                              e_unit_type::free_colonist ) } } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    f();
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( defender.type() == e_unit_type::free_colonist );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kLandDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
  }

  SECTION( "soldier->soldier, attacker wins with promotion" ) {
    combat = {
        .winner = e_combat_winner::attacker,
        .attacker =
            { .outcome =
                  EuroUnitCombatOutcome::promoted{
                      .to = UnitType::create(
                          e_unit_type::veteran_soldier ) } },
        .defender = { .outcome = EuroUnitCombatOutcome::demoted{
                          .to = UnitType::create(
                              e_unit_type::free_colonist ) } } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    // Which player's unit should get messaged.
    W.set_active_player( W.kAttackingNation );
    W.expect_msg_contains( "promoted" );
    f();
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( attacker.type() == e_unit_type::veteran_soldier );
    REQUIRE( defender.type() == e_unit_type::free_colonist );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kLandDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
  }

  SECTION(
      "soldier->free_colonist, attacker wins with capture" ) {
    combat = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = { .outcome = EuroUnitCombatOutcome::captured{
                          .new_nation = W.kAttackingNation,
                          .new_coord  = W.kLandAttack } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_unit_type::free_colonist );
    expect_combat();
    W.expect_some_animation();
    f();
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( defender.type() == e_unit_type::free_colonist );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kAttackingNation );
  }

  SECTION(
      "soldier->veteran_colonist, attacker wins with "
      "capture-and-demote" ) {
    combat = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = {
            .outcome =
                EuroUnitCombatOutcome::captured_and_demoted{
                    .to         = e_unit_type::free_colonist,
                    .new_nation = W.kAttackingNation,
                    .new_coord  = W.kLandAttack } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_unit_type::veteran_colonist );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_contains( "Veteran status lost upon capture" );
    W.set_active_player( W.kDefendingNation );
    f();
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( defender.type() == e_unit_type::free_colonist );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kAttackingNation );
  }

  SECTION( "soldier->missionary, attacker wins with destroy" ) {
    combat = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = { .outcome =
                          EuroUnitCombatOutcome::destroyed{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_unit_type::free_colonist );
    expect_combat();
    W.expect_some_animation();
    // This will ensure the "lost in battle" message box pops up.
    W.set_active_player( W.kDefendingNation );
    W.expect_msg_contains( "lost in battle" );
    f();
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.units().exists( combat.defender.id ) );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.nation() == W.kAttackingNation );
  }
}
#endif

#ifndef COMPILER_GCC
TEST_CASE( "[attack-handlers] attack_native_unit_handler" ) {
  World                 W;
  CombatEuroAttackBrave combat;
  Tribe&                tribe = W.tribe( W.kNativeTribe );
  UNWRAP_CHECK( relationship,
                tribe.relationship[W.kAttackingNation] );
  relationship.nation_has_attacked_tribe = true;
  REQUIRE( relationship.tribal_alarm == 0 );

  auto expect_combat = [&] {
    EXPECT_CALL( W.combat(),
                 euro_attack_brave(
                     W.units().unit_for( combat.attacker.id ),
                     W.units().unit_for( combat.defender.id ) ) )
        .returns( combat );
  };

  auto f = [&] {
    return W.run_handler( attack_native_unit_handler(
        W.planes(), W.ss(), W.ts(), W.player( W.active_nation_ ),
        combat.attacker.id, combat.defender.id ) );
  };

  SECTION( "ask attack, cancel" ) {
    relationship.nation_has_attacked_tribe = false;

    combat = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = {
            .outcome = NativeUnitCombatOutcome::destroyed{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_native_unit_type::brave );
    EXPECT_CALL( W.gui(), choice( _, _ ) )
        .returns<maybe<string>>( "no" );
    f();
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    NativeUnit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( defender.type == e_native_unit_type::brave );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( relationship.tribal_alarm == 0 );
  }

  SECTION( "ask attack, proceed" ) {
    relationship.nation_has_attacked_tribe = false;

    combat = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = {
            .outcome = NativeUnitCombatOutcome::destroyed{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_native_unit_type::brave );
    EXPECT_CALL( W.gui(), choice( _, _ ) )
        .returns<maybe<string>>( "yes" );
    expect_combat();
    W.expect_some_animation();
    f();
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.units().exists( combat.defender.id ) );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( relationship.tribal_alarm == 10 );
  }

  SECTION( "soldier->brave, attacker wins" ) {
    combat = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = {
            .outcome = NativeUnitCombatOutcome::destroyed{
                .tribe_retains_horses  = true,
                .tribe_retains_muskets = true } } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier,
                    e_native_unit_type::mounted_warrior );
    expect_combat();
    W.expect_some_animation();
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.muskets == 0 );
    f();
    REQUIRE( tribe.muskets == 50 );
    REQUIRE( tribe.muskets == 50 );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.units().exists( combat.defender.id ) );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( relationship.tribal_alarm == 10 );
  }

  SECTION( "soldier->brave, attacker loses, no brave change" ) {
    combat = {
        .winner = e_combat_winner::defender,
        .attacker =
            { .outcome =
                  EuroUnitCombatOutcome::demoted{
                      .to = UnitType::create(
                          e_unit_type::free_colonist ) } },
        .defender = {
            .outcome = NativeUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_native_unit_type::brave );
    expect_combat();
    W.expect_some_animation();
    f();
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    NativeUnit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( attacker.type() == e_unit_type::free_colonist );
    REQUIRE( defender.type == e_native_unit_type::brave );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.units().coord_for( defender.id ) ==
             W.kLandDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( relationship.tribal_alarm == 10 );
  }

  SECTION( "soldier->brave, attacker loses" ) {
    combat = {
        .winner = e_combat_winner::defender,
        .attacker =
            { .outcome =
                  EuroUnitCombatOutcome::demoted{
                      .to = UnitType::create(
                          e_unit_type::free_colonist ) } },
        .defender = {
            .outcome = NativeUnitCombatOutcome::promoted{
                .to = e_native_unit_type::armed_brave } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_native_unit_type::brave );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_contains( "@[H]Muskets@[] acquired by brave" );
    f();
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    NativeUnit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( attacker.type() == e_unit_type::free_colonist );
    REQUIRE( defender.type == e_native_unit_type::armed_brave );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.units().coord_for( defender.id ) ==
             W.kLandDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( relationship.tribal_alarm == 10 );
  }
}
#endif

} // namespace
} // namespace rn
