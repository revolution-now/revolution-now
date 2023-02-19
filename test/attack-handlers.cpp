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
#include "src/commodity.hpp"
#include "src/orders.hpp"
#include "src/plane-stack.hpp"

// config
#include "config/unit-type.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/native-unit.rds.hpp"
#include "src/ss/natives.hpp"
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
using ::mock::matchers::Matches;
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
    tribe.relationship[kAttackingNation].encountered = true;
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

  OrdersHandlerRunResult run_handler(
      unique_ptr<OrdersHandler> handler ) {
    wait<OrdersHandlerRunResult> const w = handler->run();
    // Use check-fails so that we can get stack traces and know
    // which test case called us.
    BASE_CHECK( !w.exception() );
    BASE_CHECK( w.ready() );
    return *w;
  }

  void expect_msg_contains( string_view msg ) {
    gui()
        .EXPECT__message_box( StrContains( string( msg ) ) )
        .returns<monostate>();
  }

  void expect_some_animation() {
    mock_land_view_plane_.EXPECT__animate( _ )
        .returns<monostate>();
  }

  void expect_convert() {
    expect_some_animation();
    gui()
        .EXPECT__message_box( StrContains( "converts" ) )
        .returns<monostate>();
    expect_some_animation();
  }

  void expect_promotion() {
    gui()
        .EXPECT__message_box( StrContains( "valor" ) )
        .returns<monostate>();
  }

  void expect_evaded() {
    gui()
        .EXPECT__message_box( StrContains( "evades" ) )
        .returns<monostate>();
  }

  void expect_ship_sunk() {
    gui()
        .EXPECT__message_box( StrContains( "sunk by" ) )
        .returns<monostate>();
  }

  void expect_ship_sunk_and_units_lost() {
    gui()
        .EXPECT__message_box(
            Matches( ".*sunk by.*been lost.*" ) )
        .returns<monostate>();
  }

  void expect_tribe_wiped_out( string_view tribe_name ) {
    gui()
        .EXPECT__message_box( fmt::format(
            "The [{}] tribe has been wiped out.", tribe_name ) )
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
  World                  W;
  OrdersHandlerRunResult expected = { .order_was_run = false };
  CombatEuroAttackEuro   combat;

  auto expect_combat = [&] {
    W.combat()
        .EXPECT__euro_attack_euro(
            W.units().unit_for( combat.attacker.id ),
            W.units().unit_for( combat.defender.id ) )
        .returns( combat );
  };

  auto f = [&] {
    return W.run_handler( attack_euro_land_handler(
        W.ss(), W.ts(), W.player( W.active_nation_ ),
        combat.attacker.id, combat.defender.id ) );
  };

  SECTION( "ships cannot attack land units" ) {
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::frigate, e_unit_type::free_colonist );
    W.expect_msg_contains( "Ships cannot attack land units" );
    REQUIRE( f() == expected );
  }

  SECTION( "non-military units cannot attack" ) {
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::free_colonist, e_unit_type::free_colonist );
    W.expect_msg_contains( "unit cannot attack" );
    REQUIRE( f() == expected );
  }

  SECTION( "cannot attack a land unit from a ship" ) {
    auto [ship_id, defender_id] = W.add_pair(
        e_unit_type::caravel, e_unit_type::free_colonist );
    combat.attacker.id =
        W.add_unit_in_cargo( e_unit_type::soldier, ship_id )
            .id();
    combat.defender.id = defender_id;
    W.expect_msg_contains(
        "cannot attack a land unit from a ship" );
    REQUIRE( f() == expected );
  }

  SECTION( "land unit cannot attack ship" ) {
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::caravel );
    W.expect_msg_contains( "Land units cannot attack ship" );
    REQUIRE( f() == expected );
  }

  SECTION( "partial movement cancelled" ) {
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    W.units()
        .unit_for( combat.attacker.id )
        .consume_mv_points( MovementPoints::_1_3() );
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        "no" );
    REQUIRE( f() == expected );
  }

  SECTION( "partial movement proceed" ) {
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    W.units()
        .unit_for( combat.attacker.id )
        .consume_mv_points( MovementPoints::_1_3() );
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        "yes" );
    expect_combat();
    W.expect_some_animation();
    expected = { .order_was_run = true };
    REQUIRE( f() == expected );
    // We're not concerned with the effects of the combat here,
    // that will be tested in other test cases.
  }
}
#endif

#ifndef COMPILER_GCC
TEST_CASE( "[attack-handlers] attack_euro_land_handler" ) {
  World                  W;
  OrdersHandlerRunResult expected = { .order_was_run = true };
  CombatEuroAttackEuro   combat;

  auto expect_combat = [&] {
    W.combat()
        .EXPECT__euro_attack_euro(
            W.units().unit_for( combat.attacker.id ),
            W.units().unit_for( combat.defender.id ) )
        .returns( combat );
  };

  auto f = [&] {
    return W.run_handler( attack_euro_land_handler(
        W.ss(), W.ts(), W.player( W.active_nation_ ),
        combat.attacker.id, combat.defender.id ) );
  };

  SECTION( "soldier->soldier, attacker loses" ) {
    combat = {
        .winner   = e_combat_winner::defender,
        .attacker = { .outcome =
                          EuroUnitCombatOutcome::demoted{
                              .to =
                                  e_unit_type::free_colonist } },
        .defender = { .outcome =
                          EuroUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    REQUIRE( W.units()
                 .unit_for( combat.attacker.id )
                 .movement_points() == 1 );
    REQUIRE( f() == expected );
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
    REQUIRE( attacker.movement_points() == 0 );
  }

  SECTION(
      "soldier->soldier, attacker loses, defender promoted" ) {
    combat = {
        .winner   = e_combat_winner::defender,
        .attacker = { .outcome =
                          EuroUnitCombatOutcome::demoted{
                              .to =
                                  e_unit_type::free_colonist } },
        .defender = {
            .outcome = EuroUnitCombatOutcome::promoted{
                .to = e_unit_type::veteran_soldier } } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    REQUIRE( f() == expected );
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
    REQUIRE( attacker.movement_points() == 0 );
  }

  SECTION(
      "soldier->soldier, attacker wins with no promotion" ) {
    combat = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = { .outcome = EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    REQUIRE( f() == expected );
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
    REQUIRE( attacker.movement_points() == 0 );
  }

  SECTION( "soldier->soldier, attacker wins with promotion" ) {
    combat = {
        .winner = e_combat_winner::attacker,
        .attacker =
            { .outcome =
                  EuroUnitCombatOutcome::promoted{
                      .to = e_unit_type::veteran_soldier } },
        .defender = { .outcome = EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    // Which player's unit should get messaged.
    W.set_active_player( W.kAttackingNation );
    W.expect_msg_contains( "promoted" );
    REQUIRE( f() == expected );
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
    REQUIRE( attacker.movement_points() == 0 );
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
    REQUIRE( f() == expected );
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
    REQUIRE( attacker.movement_points() == 0 );
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
    REQUIRE( f() == expected );
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
    REQUIRE( attacker.movement_points() == 0 );
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
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.units().exists( combat.defender.id ) );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( attacker.movement_points() == 0 );
  }
}
#endif

#ifndef COMPILER_GCC
TEST_CASE( "[attack-handlers] attack_native_unit_handler" ) {
  World                  W;
  OrdersHandlerRunResult expected = { .order_was_run = true };
  CombatEuroAttackBrave  combat;
  Tribe&                 tribe = W.tribe( W.kNativeTribe );
  TribeRelationship&     relationship =
      tribe.relationship[W.kAttackingNation];
  relationship.nation_has_attacked_tribe = true;
  REQUIRE( relationship.tribal_alarm == 0 );

  auto expect_combat = [&] {
    W.combat()
        .EXPECT__euro_attack_brave(
            W.units().unit_for( combat.attacker.id ),
            W.units().unit_for( combat.defender.id ) )
        .returns( combat );
  };

  auto f = [&] {
    return W.run_handler( attack_native_unit_handler(
        W.ss(), W.ts(), W.player( W.active_nation_ ),
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
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        "no" );
    expected = { .order_was_run = false };
    REQUIRE( f() == expected );
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
    REQUIRE( attacker.movement_points() == 1 );
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
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        "yes" );
    expect_combat();
    W.expect_some_animation();
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.units().exists( combat.defender.id ) );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( relationship.tribal_alarm == 10 );
    REQUIRE( attacker.movement_points() == 0 );
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
    REQUIRE( f() == expected );
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
    REQUIRE( attacker.movement_points() == 0 );
  }

  SECTION( "soldier->brave, attacker loses, no brave change" ) {
    combat = {
        .winner   = e_combat_winner::defender,
        .attacker = { .outcome =
                          EuroUnitCombatOutcome::demoted{
                              .to =
                                  e_unit_type::free_colonist } },
        .defender = {
            .outcome = NativeUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_native_unit_type::brave );
    expect_combat();
    W.expect_some_animation();
    REQUIRE( f() == expected );
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
    REQUIRE( attacker.movement_points() == 0 );
  }

  SECTION( "soldier->brave, attacker loses" ) {
    combat = {
        .winner   = e_combat_winner::defender,
        .attacker = { .outcome =
                          EuroUnitCombatOutcome::demoted{
                              .to =
                                  e_unit_type::free_colonist } },
        .defender = {
            .outcome = NativeUnitCombatOutcome::promoted{
                .to = e_native_unit_type::armed_brave } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_native_unit_type::brave );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_contains( "[Muskets] acquired by brave" );
    REQUIRE( f() == expected );
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
    REQUIRE( attacker.movement_points() == 0 );
  }
}
#endif

#ifndef COMPILER_GCC
TEST_CASE( "[attack-handlers] attack_dwelling_handler" ) {
  World                    W;
  OrdersHandlerRunResult   expected = { .order_was_run = true };
  CombatEuroAttackDwelling combat;
  Player&                  player = W.player( W.active_nation_ );

  Tribe&             tribe = W.tribe( W.kNativeTribe );
  TribeRelationship& relationship =
      tribe.relationship[W.kAttackingNation];
  relationship.nation_has_attacked_tribe = true;
  REQUIRE( relationship.tribal_alarm == 0 );
  Dwelling& dwelling =
      W.add_dwelling( W.kLandDefend, W.kNativeTribe );
  NativeUnitId const brave_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 2, .y = 2 }, dwelling.id )
          .id;
  W.square( W.kLandDefend ).road = true;
  dwelling.population            = 5;
  // Use only the id after the dwelling is destroyed.
  DwellingId const dwelling_id = dwelling.id;

  auto expect_combat = [&] {
    W.combat()
        .EXPECT__euro_attack_dwelling(
            W.units().unit_for( combat.attacker.id ), dwelling )
        .returns( combat );
  };

  auto f = [&] {
    return W.run_handler( attack_dwelling_handler(
        W.ss(), W.ts(), W.player( W.active_nation_ ),
        combat.attacker.id, combat.defender.id ) );
  };

  SECTION( "attacker loses" ) {
    UnitId const missionary_id =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling_id )
            .id();
    combat = {
        .winner           = e_combat_winner::defender,
        .new_tribal_alarm = 13,
        .missions_burned  = false,

        .attacker = { .outcome =
                          EuroUnitCombatOutcome::demoted{
                              .to =
                                  e_unit_type::free_colonist } },

        .defender = {
            .id      = dwelling.id,
            .outcome = DwellingCombatOutcome::no_change{} } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    expected = { .order_was_run = true };
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( attacker.type() == e_unit_type::free_colonist );
    REQUIRE( dwelling.population == 5 );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.natives().coord_for( dwelling.id ) ==
             W.kLandDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( relationship.tribal_alarm == 13 );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE(
        as_const( W.units() ).ownership_of( missionary_id ) ==
        UnitOwnership_t{
            UnitOwnership::dwelling{ .id = dwelling_id } } );
    REQUIRE( player.score_stats.dwellings_burned == 0 );
    REQUIRE( W.square( W.kLandDefend ).road == true );
    REQUIRE( W.units().exists( brave_id ) );
    REQUIRE( W.units().coord_for( brave_id ) ==
             Coord{ .x = 2, .y = 2 } );
    REQUIRE( W.natives().tribe_exists( W.kNativeTribe ) );
  }

  SECTION(
      "attacker loses, missions burned, missionary "
      "eliminated" ) {
    UnitId const missionary_id =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling_id )
            .id();
    combat = {
        .winner           = e_combat_winner::defender,
        .new_tribal_alarm = 13,
        .missions_burned  = true,

        .attacker = { .outcome =
                          EuroUnitCombatOutcome::demoted{
                              .to =
                                  e_unit_type::free_colonist } },

        .defender = {
            .id      = dwelling.id,
            .outcome = DwellingCombatOutcome::no_change{} } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    string const missions_burned_msg =
        "The [Apache] revolt against [English] "
        "missions! All English missionaries eliminated!";
    W.gui()
        .EXPECT__message_box( missions_burned_msg )
        .returns<monostate>();
    expected = { .order_was_run = true };
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( attacker.type() == e_unit_type::free_colonist );
    REQUIRE( dwelling.population == 5 );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.natives().coord_for( dwelling.id ) ==
             W.kLandDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( relationship.tribal_alarm == 13 );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( !W.units().exists( missionary_id ) );
    REQUIRE( player.score_stats.dwellings_burned == 0 );
    REQUIRE( W.square( W.kLandDefend ).road == true );
    REQUIRE( W.units().exists( brave_id ) );
    REQUIRE( W.units().coord_for( brave_id ) ==
             Coord{ .x = 2, .y = 2 } );
    REQUIRE( W.natives().tribe_exists( W.kNativeTribe ) );
  }

  SECTION(
      "attacker loses, missions burned, foreign missionary not "
      "eliminated" ) {
    UnitId const missionary_id =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling_id,
                                      W.kDefendingNation )
            .id();
    combat = {
        .winner           = e_combat_winner::defender,
        .new_tribal_alarm = 13,
        .missions_burned  = true,

        .attacker = { .outcome =
                          EuroUnitCombatOutcome::demoted{
                              .to =
                                  e_unit_type::free_colonist } },

        .defender = {
            .id      = dwelling.id,
            .outcome = DwellingCombatOutcome::no_change{} } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    string const missions_burned_msg =
        "The [Apache] revolt against [English] "
        "missions! All English missionaries eliminated!";
    W.gui()
        .EXPECT__message_box( missions_burned_msg )
        .returns<monostate>();
    expected = { .order_was_run = true };
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( attacker.type() == e_unit_type::free_colonist );
    REQUIRE( dwelling.population == 5 );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.natives().coord_for( dwelling.id ) ==
             W.kLandDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( relationship.tribal_alarm == 13 );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( W.units().exists( missionary_id ) );
    REQUIRE(
        as_const( W.units() ).ownership_of( missionary_id ) ==
        UnitOwnership_t{
            UnitOwnership::dwelling{ .id = dwelling_id } } );
    REQUIRE( player.score_stats.dwellings_burned == 0 );
    REQUIRE( W.square( W.kLandDefend ).road == true );
    REQUIRE( W.units().exists( brave_id ) );
    REQUIRE( W.units().coord_for( brave_id ) ==
             Coord{ .x = 2, .y = 2 } );
    REQUIRE( W.natives().tribe_exists( W.kNativeTribe ) );
  }

  SECTION( "attacker wins, population decrease, no convert" ) {
    UnitId const missionary_id =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling_id )
            .id();
    combat = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 13,
        .missions_burned  = false,
        .attacker         = { .outcome =
                                  EuroUnitCombatOutcome::no_change{} },
        .defender         = {
                    .id = dwelling.id,
                    .outcome =
                DwellingCombatOutcome::population_decrease{
                            .convert_produced = false } } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    expected = { .order_was_run = true };
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( dwelling.population == 4 );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.natives().coord_for( dwelling.id ) ==
             W.kLandDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( relationship.tribal_alarm == 13 );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE(
        as_const( W.units() ).ownership_of( missionary_id ) ==
        UnitOwnership_t{
            UnitOwnership::dwelling{ .id = dwelling_id } } );
    REQUIRE( player.score_stats.dwellings_burned == 0 );
    REQUIRE( W.square( W.kLandDefend ).road == true );
    REQUIRE( W.units().exists( brave_id ) );
    REQUIRE( W.units().coord_for( brave_id ) ==
             Coord{ .x = 2, .y = 2 } );
    REQUIRE( W.natives().tribe_exists( W.kNativeTribe ) );
  }

  SECTION( "attacker wins, population decrease, with convert" ) {
    UnitId const missionary_id =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling_id )
            .id();
    combat = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 13,
        .missions_burned  = false,
        .attacker         = { .outcome =
                                  EuroUnitCombatOutcome::no_change{} },
        .defender         = {
                    .id = dwelling.id,
                    .outcome =
                DwellingCombatOutcome::population_decrease{
                            .convert_produced = true } } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_convert();
    UnitId const expected_convert_id = UnitId{ 5 };

    expected = {
        .order_was_run       = true,
        .units_to_prioritize = { expected_convert_id } };
    REQUIRE( f() == expected );

    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( dwelling.population == 4 );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.natives().coord_for( dwelling.id ) ==
             W.kLandDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( relationship.tribal_alarm == 13 );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE(
        as_const( W.units() ).ownership_of( missionary_id ) ==
        UnitOwnership_t{
            UnitOwnership::dwelling{ .id = dwelling_id } } );
    REQUIRE( W.units().exists( expected_convert_id ) );
    REQUIRE( W.units().unit_for( expected_convert_id ).type() ==
             e_unit_type::native_convert );
    REQUIRE( W.units().coord_for( expected_convert_id ) ==
             W.kLandAttack );
    REQUIRE( player.score_stats.dwellings_burned == 0 );
    REQUIRE( W.square( W.kLandDefend ).road == true );
    REQUIRE( W.units().exists( brave_id ) );
    REQUIRE( W.units().coord_for( brave_id ) ==
             Coord{ .x = 2, .y = 2 } );
    REQUIRE( W.natives().tribe_exists( W.kNativeTribe ) );
  }

  SECTION( "attacker wins, dwelling burned" ) {
    dwelling.population = 1;

    combat = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 13,
        .missions_burned  = false,
        .attacker =
            { .outcome =
                  EuroUnitCombatOutcome::promoted{
                      .to = e_unit_type::veteran_soldier } },
        .defender = {
            .id      = dwelling.id,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill        = {},
                .missionary_to_release = {},
                .treasure_amount       = {},
                .tribe_destroyed       = e_tribe::apache,
                .convert_produced      = false } } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_promotion();
    W.gui()
        .EXPECT__message_box(
            "[Apache] camp burned by the "
            "[English]!" )
        .returns<monostate>();
    W.expect_tribe_wiped_out( "Apache" );

    expected = { .order_was_run       = true,
                 .units_to_prioritize = {} };
    REQUIRE( f() == expected );

    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( attacker.type() == e_unit_type::veteran_soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( player.score_stats.dwellings_burned == 1 );
    REQUIRE( W.square( W.kLandDefend ).road == false );
    REQUIRE( !W.units().exists( brave_id ) );
    REQUIRE_FALSE( W.natives().tribe_exists( W.kNativeTribe ) );
  }

  SECTION(
      "attacker wins, dwelling burned, tribe not wiped out, "
      "capital burned" ) {
    dwelling.population = 1;
    dwelling.is_capital = true;

    combat = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 13,
        .missions_burned  = false,
        .attacker =
            { .outcome =
                  EuroUnitCombatOutcome::promoted{
                      .to = e_unit_type::veteran_soldier } },
        .defender = {
            .id      = dwelling.id,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill        = {},
                .missionary_to_release = {},
                .treasure_amount       = {},
                .tribe_destroyed       = {},
                .convert_produced      = false } } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_promotion();
    W.gui()
        .EXPECT__message_box(
            "[Apache] camp burned by the "
            "[English]!" )
        .returns<monostate>();
    W.gui()
        .EXPECT__message_box(
            "The [Apache] bow before the "
            "might of the [English]!" )
        .returns<monostate>();

    expected = { .order_was_run       = true,
                 .units_to_prioritize = {} };
    REQUIRE( f() == expected );

    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( attacker.type() == e_unit_type::veteran_soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( player.score_stats.dwellings_burned == 1 );
    REQUIRE( W.square( W.kLandDefend ).road == false );
    REQUIRE( !W.units().exists( brave_id ) );
    REQUIRE( W.natives().tribe_exists( W.kNativeTribe ) );
  }

  SECTION(
      "attacker wins, dwelling burned, convert produced, "
      "missionary released, treasure produced" ) {
    UnitId const missionary_id =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling_id )
            .id();
    dwelling.population = 1;

    combat = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 13,
        .missions_burned  = false,
        .attacker =
            { .outcome =
                  EuroUnitCombatOutcome::promoted{
                      .to = e_unit_type::veteran_soldier } },
        .defender = {
            .id      = dwelling.id,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill        = {},
                .missionary_to_release = missionary_id,
                .treasure_amount       = 123,
                .tribe_destroyed       = e_tribe::apache,
                .convert_produced      = true } } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_promotion();
    W.expect_convert();
    W.gui()
        .EXPECT__message_box( fmt::format(
            "[Apache] camp burned by the [English]! "
            "[Missionary] flees in panic! Treasure worth "
            "[123] recovered from camp! It will take a "
            "[Galleon] to transport this treasure back to "
            "[London]." ) )
        .returns<monostate>();
    W.expect_some_animation(); // treasure enpixelation.
    W.expect_tribe_wiped_out( "Apache" );
    UnitId const expected_convert_id  = UnitId{ 5 };
    UnitId const expected_treasure_id = UnitId{ 6 };

    expected = {
        .order_was_run       = true,
        .units_to_prioritize = { expected_convert_id,
                                 expected_treasure_id } };
    REQUIRE( f() == expected );

    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( attacker.type() == e_unit_type::veteran_soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE(
        as_const( W.units() ).ownership_of( missionary_id ) ==
        UnitOwnership_t{
            UnitOwnership::world{ .coord = W.kLandDefend } } );
    REQUIRE( W.units().exists( expected_convert_id ) );
    REQUIRE( W.units().unit_for( expected_convert_id ).type() ==
             e_unit_type::native_convert );
    REQUIRE( W.units().coord_for( expected_convert_id ) ==
             W.kLandAttack );
    REQUIRE( W.units().exists( expected_treasure_id ) );
    REQUIRE( W.units().unit_for( expected_treasure_id ).type() ==
             e_unit_type::treasure );
    REQUIRE( W.units()
                 .unit_for( expected_treasure_id )
                 .composition()
                 .inventory()[e_unit_inventory::gold] == 123 );
    REQUIRE( W.units().coord_for( expected_treasure_id ) ==
             W.kLandDefend );
    REQUIRE( player.score_stats.dwellings_burned == 1 );
    REQUIRE( W.square( W.kLandDefend ).road == false );
    REQUIRE( !W.units().exists( brave_id ) );
    REQUIRE_FALSE( W.natives().tribe_exists( W.kNativeTribe ) );
  }

  SECTION(
      "attacker wins, dwelling burned, foreign missionary: no "
      "convert produced, missionary destroyed, capital" ) {
    UnitId const missionary_id =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling_id,
                                      W.kDefendingNation )
            .id();
    dwelling.population = 1;
    dwelling.is_capital = true;

    combat = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 13,
        .missions_burned  = false,
        .attacker =
            { .outcome =
                  EuroUnitCombatOutcome::promoted{
                      .to = e_unit_type::veteran_soldier } },
        .defender = {
            .id      = dwelling.id,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill        = {},
                .missionary_to_release = nothing,
                .treasure_amount       = nothing,
                .tribe_destroyed       = e_tribe::apache,
                .convert_produced      = false } } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_promotion();
    W.gui()
        .EXPECT__message_box( fmt::format(
            "[Apache] camp burned by the [English]! "
            "[Foreign missionary] hanged!" ) )
        .returns<monostate>();
    W.expect_tribe_wiped_out( "Apache" );

    expected = { .order_was_run       = true,
                 .units_to_prioritize = {} };
    REQUIRE( f() == expected );

    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( attacker.type() == e_unit_type::veteran_soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( !W.units().exists( missionary_id ) );
    REQUIRE( player.score_stats.dwellings_burned == 1 );
    REQUIRE( W.square( W.kLandDefend ).road == false );
    REQUIRE( !W.units().exists( brave_id ) );
    REQUIRE_FALSE( W.natives().tribe_exists( W.kNativeTribe ) );
  }
}
#endif

#ifndef COMPILER_GCC
TEST_CASE( "[attack-handlers] naval_battle_handler" ) {
  World                  W;
  CombatShipAttackShip   combat;
  OrdersHandlerRunResult expected = { .order_was_run = true };

  auto expect_combat = [&] {
    W.combat()
        .EXPECT__ship_attack_ship(
            W.units().unit_for( combat.attacker.id ),
            W.units().unit_for( combat.defender.id ) )
        .returns( combat );
  };

  auto f = [&] {
    return W.run_handler( naval_battle_handler(
        W.ss(), W.ts(), W.player( W.active_nation_ ),
        combat.attacker.id, combat.defender.id ) );
  };

  SECTION( "evade" ) {
    combat = {
        .outcome = e_naval_combat_outcome::evade,
        .winner  = nothing,
        .attacker =
            { .outcome =
                  EuroNavalUnitCombatOutcome::no_change{} },
        .defender = {
            .outcome =
                EuroNavalUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    expect_combat();
    W.expect_some_animation();
    W.expect_evaded();
    REQUIRE( W.units()
                 .unit_for( combat.attacker.id )
                 .movement_points() == 8 );
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kWaterAttack );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kWaterDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( attacker.damaged() == nothing );
    REQUIRE( defender.damaged() == nothing );
    REQUIRE( attacker.cargo().count_items() == 0 );
    REQUIRE( defender.cargo().count_items() == 0 );
  }

  SECTION( "defender damaged, sent to harbor" ) {
    combat = {
        .outcome = e_naval_combat_outcome::damaged,
        .winner  = e_combat_winner::attacker,
        .attacker =
            { .outcome =
                  EuroNavalUnitCombatOutcome::no_change{} },
        .defender = {
            .outcome = EuroNavalUnitCombatOutcome::damaged{
                .port = ShipRepairPort::european_harbor{} } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    expect_combat();
    W.expect_some_animation();
    REQUIRE( W.units()
                 .unit_for( combat.attacker.id )
                 .movement_points() == 8 );
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kWaterAttack );
    REQUIRE(
        as_const( W.units() ).ownership_of( defender.id() ) ==
        UnitOwnership_t{ UnitOwnership::harbor{
            .st = { .port_status = PortStatus::in_port{},
                    .sailed_from = nothing } } } );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( attacker.damaged() == nothing );
    REQUIRE( defender.damaged() == 0 );
    REQUIRE( attacker.cargo().count_items() == 0 );
    REQUIRE( defender.cargo().count_items() == 0 );
  }

  SECTION( "defender damaged, sent to colony" ) {
    // The colony does not need to have a drydock for this unit
    // test since we're injecting the colony to which to send the
    // damaged ship.
    Colony& colony =
        W.add_colony( { .x = 2, .y = 2 }, W.kDefendingNation );
    combat = {
        .outcome  = e_naval_combat_outcome::damaged,
        .winner   = e_combat_winner::attacker,
        .attacker = { .outcome =
                          EuroNavalUnitCombatOutcome::moved{
                              .to = W.kWaterDefend } },
        .defender = { .outcome =
                          EuroNavalUnitCombatOutcome::damaged{
                              .port = ShipRepairPort::colony{
                                  .id = colony.id } } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    expect_combat();
    W.expect_some_animation();
    REQUIRE( W.units()
                 .unit_for( combat.attacker.id )
                 .movement_points() == 8 );
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kWaterDefend );
    REQUIRE(
        as_const( W.units() ).ownership_of( defender.id() ) ==
        UnitOwnership_t{ UnitOwnership::world{
            .coord = { .x = 2, .y = 2 } } } );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( attacker.damaged() == nothing );
    REQUIRE( defender.damaged() == 0 );
    REQUIRE( attacker.cargo().count_items() == 0 );
    REQUIRE( defender.cargo().count_items() == 0 );
  }

  SECTION( "attacker sunk" ) {
    combat = {
        .outcome  = e_naval_combat_outcome::sunk,
        .winner   = e_combat_winner::defender,
        .attacker = { .outcome =
                          EuroNavalUnitCombatOutcome::sunk{} },
        .defender = {
            .outcome =
                EuroNavalUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    expect_combat();
    W.expect_some_animation();
    W.expect_ship_sunk();
    REQUIRE( W.units()
                 .unit_for( combat.attacker.id )
                 .movement_points() == 8 );
    REQUIRE( f() == expected );
    REQUIRE_FALSE( W.units().exists( combat.attacker.id ) );
    // !! attacker unit does not exist here.
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kWaterDefend );
    REQUIRE( defender.nation() == W.kDefendingNation );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( defender.damaged() == nothing );
  }

  SECTION( "attacker sunk containing units" ) {
    combat = {
        .outcome  = e_naval_combat_outcome::sunk,
        .winner   = e_combat_winner::defender,
        .attacker = { .outcome =
                          EuroNavalUnitCombatOutcome::sunk{} },
        .defender = {
            .outcome =
                EuroNavalUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    UnitId const free_colonist_id =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             attacker.id() )
            .id();
    UnitId const soldier_id =
        W.add_unit_in_cargo( e_unit_type::soldier,
                             attacker.id() )
            .id();
    REQUIRE( attacker.cargo().count_items() == 2 );
    expect_combat();
    W.expect_some_animation();
    W.expect_ship_sunk_and_units_lost();
    REQUIRE( W.units()
                 .unit_for( combat.attacker.id )
                 .movement_points() == 8 );
    REQUIRE( W.units().exists( combat.attacker.id ) );
    REQUIRE( W.units().exists( free_colonist_id ) );
    REQUIRE( W.units().exists( soldier_id ) );
    REQUIRE( f() == expected );
    REQUIRE_FALSE( W.units().exists( combat.attacker.id ) );
    REQUIRE_FALSE( W.units().exists( free_colonist_id ) );
    REQUIRE_FALSE( W.units().exists( soldier_id ) );
    // !! attacker unit and its cargo units do not exist here.
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kWaterDefend );
    REQUIRE( defender.nation() == W.kDefendingNation );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( defender.damaged() == nothing );
    REQUIRE( defender.cargo().count_items() == 0 );
  }

  // NOTE: this is temporary until we implement the UI routine
  // which allows the winner to capture the loser's commodities.
  SECTION( "attacker damaged with commodity cargo" ) {
    combat = {
        .outcome = e_naval_combat_outcome::damaged,
        .winner  = e_combat_winner::defender,
        .attacker =
            { .outcome =
                  EuroNavalUnitCombatOutcome::damaged{
                      .port =
                          ShipRepairPort::european_harbor{} } },
        .defender = {
            .outcome =
                EuroNavalUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    add_commodity_to_cargo(
        W.units(),
        Commodity{ .type = e_commodity::ore, .quantity = 10 },
        attacker.id(), /*slot=*/0,
        /*try_other_slots=*/false );
    add_commodity_to_cargo(
        W.units(),
        Commodity{ .type = e_commodity::lumber, .quantity = 20 },
        attacker.id(), /*slot=*/1,
        /*try_other_slots=*/false );
    add_commodity_to_cargo(
        W.units(),
        Commodity{ .type = e_commodity::ore, .quantity = 10 },
        defender.id(), /*slot=*/0,
        /*try_other_slots=*/false );
    REQUIRE( attacker.cargo().count_items() == 2 );
    REQUIRE( defender.cargo().count_items() == 1 );
    expect_combat();
    W.expect_some_animation();
    W.gui()
        .EXPECT__message_box(
            "English [Privateer] damaged in battle! Ship sent "
            "to [London] for repair." )
        .returns<monostate>();
    REQUIRE( W.units()
                 .unit_for( combat.attacker.id )
                 .movement_points() == 8 );
    REQUIRE( f() == expected );
    REQUIRE( W.units().exists( combat.attacker.id ) );
    REQUIRE( W.units().exists( combat.defender.id ) );
    REQUIRE(
        as_const( W.units() ).ownership_of( attacker.id() ) ==
        UnitOwnership_t{ UnitOwnership::harbor{
            .st = { .port_status = PortStatus::in_port{},
                    .sailed_from = nothing } } } );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kWaterDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( attacker.damaged() == 0 );
    REQUIRE( defender.damaged() == nothing );
    REQUIRE( attacker.cargo().count_items() == 0 );
    REQUIRE( defender.cargo().count_items() == 1 );
  }

  SECTION( "attacker damaged with unit cargo" ) {
    combat = {
        .outcome = e_naval_combat_outcome::damaged,
        .winner  = e_combat_winner::defender,
        .attacker =
            { .outcome =
                  EuroNavalUnitCombatOutcome::damaged{
                      .port =
                          ShipRepairPort::european_harbor{} } },
        .defender = {
            .outcome =
                EuroNavalUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    UnitId const free_colonist_id =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             attacker.id() )
            .id();
    UnitId const soldier_id =
        W.add_unit_in_cargo( e_unit_type::soldier,
                             attacker.id() )
            .id();
    REQUIRE( attacker.cargo().count_items() == 2 );
    expect_combat();
    W.expect_some_animation();
    W.gui()
        .EXPECT__message_box(
            "English [Privateer] damaged in battle! Ship sent "
            "to [London] for repair. [Two] units onboard have "
            "been lost." )
        .returns<monostate>();
    REQUIRE( W.units()
                 .unit_for( combat.attacker.id )
                 .movement_points() == 8 );
    REQUIRE( W.units().exists( free_colonist_id ) );
    REQUIRE( W.units().exists( soldier_id ) );
    REQUIRE( f() == expected );
    REQUIRE( W.units().exists( combat.attacker.id ) );
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE_FALSE( W.units().exists( free_colonist_id ) );
    REQUIRE_FALSE( W.units().exists( soldier_id ) );
    REQUIRE(
        as_const( W.units() ).ownership_of( attacker.id() ) ==
        UnitOwnership_t{ UnitOwnership::harbor{
            .st = { .port_status = PortStatus::in_port{},
                    .sailed_from = nothing } } } );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kWaterDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( attacker.damaged() == 0 );
    REQUIRE( defender.damaged() == nothing );
    REQUIRE( attacker.cargo().count_items() == 0 );
    REQUIRE( defender.cargo().count_items() == 0 );
  }
}
#endif

#ifndef COMPILER_GCC
TEST_CASE(
    "[attack-handlers] attack_colony_undefended_handler" ) {
  World                            W;
  CombatEuroAttackUndefendedColony combat;
  // TODO
}
#endif

} // namespace
} // namespace rn
