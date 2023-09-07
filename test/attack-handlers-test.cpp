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
#include "test/mocks/ieuro-mind.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/inative-mind.hpp"
#include "test/mocks/land-view-plane.hpp"

// Revolution Now
#include "src/command.hpp"
#include "src/commodity.hpp"
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
using ::mock::matchers::AllOf;
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

  CommandHandlerRunResult run_handler(
      unique_ptr<CommandHandler> handler ) {
    wait<CommandHandlerRunResult> const w = handler->run();
    // Use check-fails so that we can get stack traces and know
    // which test case called us.
    BASE_CHECK( !w.exception() );
    BASE_CHECK( w.ready() );
    return *w;
  }

  // NOTE: The idea in this module is that we expect exact mes-
  // sages when those messages are generated directly by this
  // module. But when the messages are generated indirectly by
  // another module (which has its own tests) then we prefer just
  // to use the "contains" variants further below to both avoid
  // redundant tests and to make these tests less fragile against
  // changes to those messages.
  void expect_msg_equals( e_nation nation, string_view msg ) {
    euro_mind( nation )
        .EXPECT__message_box( string( msg ) )
        .returns<monostate>();
  }

  template<typename... Args>
  void expect_msg_contains( e_nation nation,
                            Args&&... fragments ) {
    euro_mind( nation )
        .EXPECT__message_box(
            AllOf( StrContains( string( fragments ) )... ) )
        .template returns<monostate>();
  }

  template<typename... Args>
  void expect_msg_both_contains( Args&&... fragments ) {
    expect_msg_contains( kAttackingNation,
                         std::forward<Args>( fragments )... );
    expect_msg_contains( kDefendingNation,
                         std::forward<Args>( fragments )... );
  }

  template<typename... Args>
  void expect_msg_contains( e_tribe tribe_type,
                            Args&&... fragments ) {
    native_mind( tribe_type )
        .EXPECT__message_box(
            AllOf( StrContains( string( fragments ) )... ) )
        .template returns<monostate>();
  }

  void expect_some_animation() {
    mock_land_view_plane_.EXPECT__animate( _ )
        .returns<monostate>();
  }

  void expect_convert() {
    expect_some_animation();
    expect_msg_contains( kAttackingNation, "[converts]" );
    expect_some_animation();
  }

  void expect_promotion( e_nation nation ) {
    expect_msg_contains( nation, "victory" );
  }

  void expect_evaded( e_nation nation ) {
    expect_msg_contains( nation, "evades" );
  }

  void expect_ship_sunk( e_nation nation ) {
    expect_msg_contains( nation, "sunk by" );
  }

  void expect_tribe_wiped_out( string_view tribe_name ) {
    gui()
        .EXPECT__message_box( fmt::format(
            "The [{}] tribe has been wiped out.", tribe_name ) )
        .returns<monostate>();
  }

  MockLandViewPlane mock_land_view_plane_;
};

/****************************************************************
** Test Cases
*****************************************************************/
// This test case tests failure modes that are common to most of
// the handlers.
#ifndef COMPILER_GCC
TEST_CASE( "[attack-handlers] common failure checks" ) {
  World                   W;
  CommandHandlerRunResult expected = { .order_was_run = false };
  CombatEuroAttackEuro    combat;

  auto expect_combat = [&] {
    W.combat()
        .EXPECT__euro_attack_euro(
            W.units().unit_for( combat.attacker.id ),
            W.units().unit_for( combat.defender.id ) )
        .returns( combat );
  };

  auto f = [&] {
    return W.run_handler( attack_euro_land_handler(
        W.ss(), W.ts(), combat.attacker.id,
        combat.defender.id ) );
  };

  SECTION( "ships cannot attack land units" ) {
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::frigate, e_unit_type::free_colonist );
    W.expect_msg_contains( W.kAttackingNation,
                           "Ships cannot attack land units" );
    REQUIRE( f() == expected );
  }

  SECTION( "non-military units cannot attack" ) {
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::free_colonist, e_unit_type::free_colonist );
    W.expect_msg_contains( W.kAttackingNation,
                           "unit cannot attack" );
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
        W.kAttackingNation,
        "cannot attack a land unit from a ship" );
    REQUIRE( f() == expected );
  }

  SECTION( "land unit cannot attack ship" ) {
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::caravel );
    W.expect_msg_contains( W.kAttackingNation,
                           "Land units cannot attack ship" );
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
    W.expect_msg_contains( W.kAttackingNation, "French",
                           "Soldier", "defeats" );
    W.expect_msg_contains( W.kDefendingNation, "French",
                           "Soldier", "defeats" );
    expected = { .order_was_run = true };
    REQUIRE( f() == expected );
    // We're not concerned with the effects of the combat here,
    // that will be tested in other test cases.
  }
}
#endif

#ifndef COMPILER_GCC
TEST_CASE( "[attack-handlers] attack_euro_land_handler" ) {
  World                   W;
  CommandHandlerRunResult expected = { .order_was_run = true };
  CombatEuroAttackEuro    combat;

  auto expect_combat = [&] {
    W.combat()
        .EXPECT__euro_attack_euro(
            W.units().unit_for( combat.attacker.id ),
            W.units().unit_for( combat.defender.id ) )
        .returns( combat );
  };

  auto f = [&] {
    return W.run_handler( attack_euro_land_handler(
        W.ss(), W.ts(), combat.attacker.id,
        combat.defender.id ) );
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
    W.expect_msg_both_contains( "French", "defeats",
                                "wilderness" );
    W.expect_msg_contains( W.kAttackingNation, "English",
                           "Soldier", "routed" );
    W.expect_msg_contains( W.kDefendingNation, "English",
                           "Soldier", "routed" );
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
    W.expect_msg_both_contains( "French", "defeats",
                                "wilderness" );
    W.expect_msg_contains( W.kAttackingNation, "English",
                           "Soldier", "routed" );
    W.expect_msg_contains( W.kDefendingNation, "promoted" );
    W.expect_msg_contains( W.kDefendingNation, "routed" );
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
    W.expect_msg_both_contains( "English", "defeats",
                                "wilderness" );
    W.expect_msg_contains( W.kAttackingNation, "French",
                           "Soldier", "routed" );
    W.expect_msg_contains( W.kDefendingNation, "French",
                           "Soldier", "routed" );
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
    W.expect_msg_both_contains( "English", "defeats",
                                "wilderness" );
    W.expect_msg_contains( W.kAttackingNation, "promoted" );
    W.expect_msg_contains( W.kAttackingNation, "routed" );
    W.expect_msg_contains( W.kDefendingNation, "routed" );
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
    W.expect_msg_both_contains( "English", "defeats",
                                "wilderness" );
    W.expect_msg_contains( W.kAttackingNation, "captured" );
    W.expect_msg_contains( W.kDefendingNation, "captured" );
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
    W.expect_msg_both_contains( "English", "defeats",
                                "wilderness" );
    W.expect_msg_contains( W.kDefendingNation, "captured" );
    W.expect_msg_contains( W.kAttackingNation, "captured" );
    W.expect_msg_contains( W.kAttackingNation,
                           "Veteran status lost" );
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
    W.expect_msg_both_contains( "English", "defeats",
                                "wilderness" );
    W.expect_msg_contains( W.kDefendingNation,
                           "lost in battle" );
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
  World                   W;
  CommandHandlerRunResult expected = { .order_was_run = true };
  CombatEuroAttackBrave   combat;
  Tribe&                  tribe = W.tribe( W.kNativeTribe );
  TribeRelationship&      relationship =
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
        W.ss(), W.ts(), combat.attacker.id,
        combat.defender.id ) );
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
    W.expect_msg_contains( W.kAttackingNation, "English",
                           "Soldier", "routed" );
    W.expect_msg_contains( W.kNativeTribe, "English", "Soldier",
                           "routed" );
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
    W.expect_msg_contains( W.kAttackingNation, "routed" );
    W.expect_msg_contains( W.kAttackingNation, "Brave",
                           "muskets" );
    W.expect_msg_contains( W.kNativeTribe, "Brave", "muskets" );
    W.expect_msg_contains( W.kNativeTribe, "routed" );
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
  CommandHandlerRunResult  expected = { .order_was_run = true };
  CombatEuroAttackDwelling combat;
  Player& player = W.player( W.kAttackingNation );

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
        W.ss(), W.ts(), combat.attacker.id,
        combat.defender.id ) );
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
    W.expect_msg_contains( W.kAttackingNation, "routed" );
    W.expect_msg_contains( W.kNativeTribe, "routed" );
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

        UnitOwnership::dwelling{ .id = dwelling_id } );
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
    W.expect_msg_equals(
        W.kAttackingNation,
        "The [Apache] revolt against [English] missions! All "
        "English missionaries eliminated!" );
    W.expect_msg_contains( W.kAttackingNation, "routed" );
    W.expect_msg_contains( W.kNativeTribe, "routed" );
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
    W.expect_msg_equals(
        W.kAttackingNation,
        "The [Apache] revolt against [English] missions! All "
        "English missionaries eliminated!" );
    W.expect_msg_contains( W.kAttackingNation, "routed" );
    W.expect_msg_contains( W.kNativeTribe, "routed" );
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

        UnitOwnership::dwelling{ .id = dwelling_id } );
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

        UnitOwnership::dwelling{ .id = dwelling_id } );
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

        UnitOwnership::dwelling{ .id = dwelling_id } );
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
    W.expect_promotion( W.kAttackingNation );
    W.expect_msg_equals(
        W.kAttackingNation,
        "[Apache] camp burned by the [English]!" );
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
    W.expect_promotion( W.kAttackingNation );
    W.expect_msg_equals(
        W.kAttackingNation,
        "[Apache] capital burned by the [English]!" );
    W.expect_msg_equals(
        W.kAttackingNation,
        "The [Apache] bow before the might of the [English]!" );

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
    W.expect_promotion( W.kAttackingNation );
    W.expect_convert();
    W.expect_msg_equals(
        W.kAttackingNation,
        fmt::format( "[Apache] camp burned by the [English]! "
                     "[Missionary] flees in panic! Treasure "
                     "worth [123\x7f] has been recovered! It "
                     "will take a [Galleon] to transport this "
                     "treasure back to [London]." ) );
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

        UnitOwnership::world{ .coord = W.kLandDefend } );
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
    W.expect_promotion( W.kAttackingNation );
    W.expect_msg_equals(
        W.kAttackingNation,
        fmt::format( "[Apache] capital burned by the [English]! "
                     "[Foreign missionary] hanged!" ) );
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
  World                   W;
  CombatShipAttackShip    combat;
  CommandHandlerRunResult expected = { .order_was_run = true };

  auto expect_combat = [&] {
    W.combat()
        .EXPECT__ship_attack_ship(
            W.units().unit_for( combat.attacker.id ),
            W.units().unit_for( combat.defender.id ) )
        .returns( combat );
  };

  auto f = [&] {
    return W.run_handler(
        naval_battle_handler( W.ss(), W.ts(), combat.attacker.id,
                              combat.defender.id ) );
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
    W.expect_evaded( W.kAttackingNation );
    W.expect_evaded( W.kDefendingNation );
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
    REQUIRE( !attacker.orders().holds<unit_orders::damaged>() );
    REQUIRE( !defender.orders().holds<unit_orders::damaged>() );
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
    W.expect_msg_contains( W.kAttackingNation, "damaged",
                           "La Rochelle", "repair" );
    W.expect_msg_contains( W.kDefendingNation, "damaged",
                           "La Rochelle", "repair" );
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    Unit const& defender =
        W.units().unit_for( combat.defender.id );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kWaterAttack );
    REQUIRE(
        as_const( W.units() ).ownership_of( defender.id() ) ==
        UnitOwnership::harbor{
            .st = { .port_status = PortStatus::in_port{},
                    .sailed_from = nothing } } );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( !attacker.orders().holds<unit_orders::damaged>() );
    REQUIRE( defender.orders() ==
             unit_orders{ unit_orders::damaged{
                 .turns_until_repair = 6 } } );
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
    W.expect_msg_contains( W.kAttackingNation, "Merchantman",
                           "damaged", "1" );
    W.expect_msg_contains( W.kDefendingNation, "Merchantman",
                           "damaged", "1" );
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
        UnitOwnership::world{ .coord = { .x = 2, .y = 2 } } );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( !attacker.orders().holds<unit_orders::damaged>() );
    REQUIRE( defender.orders() ==
             unit_orders{ unit_orders::damaged{
                 .turns_until_repair = 2 } } );
    REQUIRE( attacker.cargo().count_items() == 0 );
    REQUIRE( defender.cargo().count_items() == 0 );
  }

  SECTION( "defender damaged, sent to colony (caravel)" ) {
    // The caravel, when sent to a drydock for repair, requires
    // no turns for repair.
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
        e_unit_type::privateer, e_unit_type::caravel );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_contains( W.kAttackingNation, "Caravel",
                           "damaged", "1" );
    W.expect_msg_contains( W.kDefendingNation, "Caravel",
                           "damaged", "1" );
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
        UnitOwnership::world{ .coord = { .x = 2, .y = 2 } } );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 4 );
    REQUIRE( !attacker.orders().holds<unit_orders::damaged>() );
    REQUIRE( defender.orders() ==
             unit_orders{ unit_orders::none{} } );
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
    W.expect_ship_sunk( W.kAttackingNation );
    W.expect_ship_sunk( W.kDefendingNation );
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
    REQUIRE( !defender.orders().holds<unit_orders::damaged>() );
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
    W.expect_msg_contains( W.kAttackingNation, "sunk" );
    W.expect_msg_contains( W.kAttackingNation, "Two", "units",
                           "lost" );
    W.expect_ship_sunk( W.kDefendingNation );
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
    REQUIRE( !defender.orders().holds<unit_orders::damaged>() );
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
    W.expect_msg_contains( W.kAttackingNation, "Privateer",
                           "damaged", "London" );
    W.expect_msg_contains( W.kDefendingNation, "Privateer",
                           "damaged", "London" );
    REQUIRE( W.units()
                 .unit_for( combat.attacker.id )
                 .movement_points() == 8 );
    REQUIRE( f() == expected );
    REQUIRE( W.units().exists( combat.attacker.id ) );
    REQUIRE( W.units().exists( combat.defender.id ) );
    REQUIRE(
        as_const( W.units() ).ownership_of( attacker.id() ) ==
        UnitOwnership::harbor{
            .st = { .port_status = PortStatus::in_port{},
                    .sailed_from = nothing } } );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kWaterDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( attacker.orders() ==
             unit_orders{ unit_orders::damaged{
                 .turns_until_repair = 8 } } );
    REQUIRE( !defender.orders().holds<unit_orders::damaged>() );
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
    W.expect_msg_contains( W.kAttackingNation, "Privateer",
                           "damaged", "London" );
    W.expect_msg_contains( W.kAttackingNation, "Two",
                           "onboard" );
    W.expect_msg_contains( W.kDefendingNation, "Privateer",
                           "damaged", "London" );
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
        UnitOwnership::harbor{
            .st = { .port_status = PortStatus::in_port{},
                    .sailed_from = nothing } } );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kWaterDefend );
    REQUIRE( attacker.nation() == W.kAttackingNation );
    REQUIRE( defender.nation() == W.kDefendingNation );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( attacker.orders() ==
             unit_orders{ unit_orders::damaged{
                 .turns_until_repair = 8 } } );
    REQUIRE( !defender.orders().holds<unit_orders::damaged>() );
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
