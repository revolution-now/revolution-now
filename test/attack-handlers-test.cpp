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
#include "test/mocks/ieuro-agent.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/inative-agent.hpp"
#include "test/mocks/land-view-plane.hpp"

// Revolution Now
#include "src/command.hpp"
#include "src/commodity.hpp"
#include "src/imap-updater.hpp"
#include "src/plane-stack.hpp"
#include "src/visibility.hpp"

// config
#include "config/unit-type.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/land-view.rds.hpp"
#include "src/ss/native-unit.rds.hpp"
#include "src/ss/natives.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/tribe.rds.hpp"
#include "src/ss/unit.hpp"
#include "src/ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::gfx::point;
using ::mock::matchers::_;
using ::mock::matchers::AllOf;
using ::mock::matchers::Field;
using ::mock::matchers::Property;
using ::mock::matchers::StrContains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;

  static inline e_player const kAttackingPlayer =
      e_player::english;
  static inline e_player const kDefendingPlayer =
      e_player::french;
  static inline e_player const kThirdPlayer = e_player::spanish;

  static inline e_tribe const kNativeTribe = e_tribe::apache;

  World() : Base() {
    add_player( kAttackingPlayer );
    add_player( kDefendingPlayer );
    add_player( kThirdPlayer );
    set_human_player_and_rest_ai( kAttackingPlayer );
    common_player_init( player( kAttackingPlayer ) );
    common_player_init( player( kDefendingPlayer ) );
    set_default_player_type( kAttackingPlayer );
    planes().get().set_bottom<ILandViewPlane>(
        mock_land_view_plane_ );
    Tribe& tribe = add_tribe( kNativeTribe );
    tribe.relationship[kAttackingPlayer].encountered = true;
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

  UnitId add_attacker( e_unit_type type, e_player const player =
                                             kAttackingPlayer ) {
    if( unit_attr( type ).ship )
      return add_unit_on_map( type, kWaterAttack, player ).id();
    else
      return add_unit_on_map( type, kLandAttack, player ).id();
  }

  UnitId add_defender( e_unit_type type ) {
    if( unit_attr( type ).ship )
      return add_unit_on_map( type, kWaterDefend,
                              kDefendingPlayer )
          .id();
    else
      return add_unit_on_map( type, kLandDefend,
                              kDefendingPlayer )
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
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
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

  void create_land_9x9() {
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
    // 0  1  2  3  4  5  6  7  8
       L, L, L, L, L, L, L, L, L, // 0
       L, L, L, L, L, L, L, L, L, // 1
       L, L, L, L, L, L, L, L, L, // 2
       L, L, L, L, L, L, L, L, L, // 3
       L, L, L, L, L, L, L, L, L, // 4
       L, L, L, L, L, L, L, L, L, // 5
       L, L, L, L, L, L, L, L, L, // 6
       L, L, L, L, L, L, L, L, L, // 7
       L, L, L, L, L, L, L, L, L, // 8
    };
    // clang-format on
    build_map( std::move( tiles ), 9 );
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
  void expect_msg_equals( e_player player, string_view msg ) {
    euro_agent( player ).EXPECT__message_box( string( msg ) );
  }

  template<typename... Args>
  void expect_msg_contains( e_player player,
                            Args&&... fragments ) {
    euro_agent( player ).EXPECT__message_box(
        AllOf( StrContains( string( fragments ) )... ) );
  }

  template<typename... Args>
  void expect_msg_contains( e_tribe tribe_type,
                            Args&&... fragments ) {
    native_agent( tribe_type )
        .EXPECT__message_box(
            AllOf( StrContains( string( fragments ) )... ) );
  }

  void expect_some_animation() {
    mock_land_view_plane_.EXPECT__animate( _ );
  }

  void expect_convert() {
    expect_msg_contains( kAttackingPlayer, "[converts]" );
    expect_some_animation();
  }

  void expect_rebel_convert() {
    expect_msg_contains( kAttackingPlayer, "[converts]",
                         "Rebel" );
    expect_some_animation();
  }

  void expect_promotion( e_player player ) {
    expect_msg_contains( player, "victory" );
  }

  void expect_evaded( e_player player ) {
    expect_msg_contains( player, "evades" );
  }

  void expect_ship_sunk( e_player player ) {
    expect_msg_contains( player, "sunk by" );
  }

  void expect_tribe_wiped_out(
      string_view tribe_name,
      e_player const player = kAttackingPlayer ) {
    euro_agent( player ).EXPECT__message_box( fmt::format(
        "The [{}] tribe has been wiped out.", tribe_name ) );
  }

  void expect_unit_captures_cargo(
      UnitId const src, UnitId const dst,
      CapturableCargo const& capturable ) {
    Unit& dst_unit              = units().unit_for( dst );
    e_player const taker_player = dst_unit.player_type();
    auto& taker_agent           = euro_agent( taker_player );
    taker_agent
        .EXPECT__select_commodities_to_capture( src, dst,
                                                capturable )
        .returns<base::heap_value<CapturableCargoItems>>(
            capturable.items );
  }

  [[nodiscard]] auto expect_attacking_player() {
    return Field( &Player::type, kAttackingPlayer );
  }

  [[nodiscard]] auto expect_defending_player() {
    return Field( &Player::type, kDefendingPlayer );
  }

  [[nodiscard]] auto expect_unit_of_type(
      e_unit_type const type ) {
    return Property( &Unit::type, type );
  }

  MockLandViewPlane mock_land_view_plane_;
};

/****************************************************************
** Test Cases
*****************************************************************/
// This test case tests failure modes that are common to most of
// the handlers for the case of a land battle.
TEST_CASE(
    "[attack-handlers] common failure checks (land attacker)" ) {
  World W;
  W.create_default_map();
  CommandHandlerRunResult expected = { .order_was_run = false };
  CombatEuroAttackEuro combat;
  MockIEuroAgent& attacker_agent =
      W.euro_agent( W.kAttackingPlayer );

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

  SECTION( "non-military units cannot attack" ) {
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::free_colonist, e_unit_type::free_colonist );
    W.expect_msg_contains( W.kAttackingPlayer,
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
        W.kAttackingPlayer,
        "cannot attack a land unit from a ship" );
    REQUIRE( f() == expected );
  }

  SECTION( "land unit cannot attack ship" ) {
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::caravel );
    W.expect_msg_contains( W.kAttackingPlayer,
                           "Our land units can neither attack "
                           "nor board foreign ships." );
    REQUIRE( f() == expected );
  }

  SECTION( "land unit cannot board ship" ) {
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::free_colonist, e_unit_type::caravel );
    W.expect_msg_contains( W.kAttackingPlayer,
                           "Our land units can neither attack "
                           "nor board foreign ships." );
    REQUIRE( f() == expected );
  }

  SECTION( "partial movement cancelled" ) {
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    W.units()
        .unit_for( combat.attacker.id )
        .consume_mv_points( MovementPoints::_1_3() );
    attacker_agent
        .EXPECT__attack_with_partial_movement_points(
            combat.attacker.id )
        .returns( ui::e_confirm::no );
    REQUIRE( f() == expected );
  }

  SECTION( "partial movement proceed" ) {
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    W.units()
        .unit_for( combat.attacker.id )
        .consume_mv_points( MovementPoints::_1_3() );
    attacker_agent
        .EXPECT__attack_with_partial_movement_points(
            combat.attacker.id )
        .returns( ui::e_confirm::yes );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_contains( W.kDefendingPlayer, "French",
                           "Soldier", "defeats" );
    expected = { .order_was_run = true };
    REQUIRE( f() == expected );
    // We're not concerned with the effects of the combat here,
    // that will be tested in other test cases.
  }
}

// This test case tests failure modes that are common to most of
// the handlers for the case of a ship battle.
TEST_CASE(
    "[attack-handlers] common failure checks (ship attacker)" ) {
  World W;
  W.create_default_map();
  CommandHandlerRunResult expected = { .order_was_run = false };
  CombatEuroAttackEuro combat;

  auto f = [&] {
    return W.run_handler(
        naval_battle_handler( W.ss(), W.ts(), combat.attacker.id,
                              combat.defender.id ) );
  };

  SECTION( "ships cannot attack land units" ) {
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::frigate, e_unit_type::free_colonist );
    W.expect_msg_contains( W.kAttackingPlayer,
                           "Ships cannot attack land units" );
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[attack-handlers] attack_euro_land_handler" ) {
  World W;
  W.create_default_map();
  CommandHandlerRunResult expected = { .order_was_run = true };
  CombatEuroAttackEuro combat;

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
                          .to = e_unit_type::free_colonist } },
      .defender = { .outcome =
                        EuroUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    REQUIRE( W.units()
                 .unit_for( combat.attacker.id )
                 .movement_points() == 1 );
    W.expect_msg_contains( W.kAttackingPlayer, "English",
                           "Soldier", "routed" );
    W.expect_msg_contains( W.kDefendingPlayer, "English",
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( defender.player_type() == W.kDefendingPlayer );
    REQUIRE( attacker.movement_points() == 0 );
  }

  SECTION(
      "soldier->soldier, attacker loses, defender promoted" ) {
    combat = {
      .winner   = e_combat_winner::defender,
      .attacker = { .outcome =
                        EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } },
      .defender = { .outcome = EuroUnitCombatOutcome::promoted{
                      .to = e_unit_type::veteran_soldier } } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_contains( W.kAttackingPlayer, "English",
                           "Soldier", "routed" );
    W.expect_msg_contains( W.kDefendingPlayer, "promoted" );
    W.expect_msg_contains( W.kDefendingPlayer, "routed" );
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( defender.player_type() == W.kDefendingPlayer );
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
    W.expect_msg_contains( W.kAttackingPlayer, "French",
                           "Soldier", "routed" );
    W.expect_msg_contains( W.kDefendingPlayer, "French",
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( defender.player_type() == W.kDefendingPlayer );
    REQUIRE( attacker.movement_points() == 0 );
  }

  SECTION( "soldier->soldier, attacker wins with promotion" ) {
    combat = {
      .winner   = e_combat_winner::attacker,
      .attacker = { .outcome =
                        EuroUnitCombatOutcome::promoted{
                          .to = e_unit_type::veteran_soldier } },
      .defender = { .outcome = EuroUnitCombatOutcome::demoted{
                      .to = e_unit_type::free_colonist } } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier, e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_contains( W.kAttackingPlayer, "promoted" );
    W.expect_msg_contains( W.kAttackingPlayer, "routed" );
    W.expect_msg_contains( W.kDefendingPlayer, "routed" );
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( defender.player_type() == W.kDefendingPlayer );
    REQUIRE( attacker.movement_points() == 0 );
  }

  SECTION(
      "soldier->free_colonist, attacker wins with capture" ) {
    combat = {
      .winner   = e_combat_winner::attacker,
      .attacker = { .outcome =
                        EuroUnitCombatOutcome::no_change{} },
      .defender = { .outcome = EuroUnitCombatOutcome::captured{
                      .new_player = W.kAttackingPlayer,
                      .new_coord  = W.kLandAttack } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_unit_type::free_colonist );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_contains( W.kAttackingPlayer, "captured" );
    W.expect_msg_contains( W.kDefendingPlayer, "captured" );
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( defender.player_type() == W.kAttackingPlayer );
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
        .outcome = EuroUnitCombatOutcome::captured_and_demoted{
          .to         = e_unit_type::free_colonist,
          .new_player = W.kAttackingPlayer,
          .new_coord  = W.kLandAttack } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_unit_type::veteran_colonist );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_contains( W.kDefendingPlayer, "captured" );
    W.expect_msg_contains( W.kAttackingPlayer, "captured" );
    W.expect_msg_contains( W.kAttackingPlayer,
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( defender.player_type() == W.kAttackingPlayer );
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
    W.expect_msg_contains( W.kDefendingPlayer,
                           "lost in battle" );
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.units().exists( combat.defender.id ) );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( attacker.movement_points() == 0 );
  }
}

TEST_CASE( "[attack-handlers] attack_native_unit_handler" ) {
  World W;
  W.create_default_map();
  CommandHandlerRunResult expected = { .order_was_run = true };
  CombatEuroAttackBrave combat;
  Tribe& tribe = W.tribe( W.kNativeTribe );
  TribeRelationship& relationship =
      tribe.relationship[W.kAttackingPlayer];
  relationship.player_has_attacked_tribe = true;
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
    relationship.player_has_attacked_tribe = false;

    combat = {
      .winner   = e_combat_winner::attacker,
      .attacker = { .outcome =
                        EuroUnitCombatOutcome::no_change{} },
      .defender = { .outcome =
                        NativeUnitCombatOutcome::destroyed{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_native_unit_type::brave );
    W.euro_agent( W.kAttackingPlayer )
        .EXPECT__should_attack_natives( W.kNativeTribe )
        .returns( ui::e_confirm::no );
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 0 );
    REQUIRE( attacker.movement_points() == 1 );
  }

  SECTION( "ask attack, proceed" ) {
    relationship.player_has_attacked_tribe = false;

    combat = {
      .winner   = e_combat_winner::attacker,
      .attacker = { .outcome =
                        EuroUnitCombatOutcome::no_change{} },
      .defender = { .outcome =
                        NativeUnitCombatOutcome::destroyed{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_native_unit_type::brave );
    W.euro_agent( W.kAttackingPlayer )
        .EXPECT__should_attack_natives( W.kNativeTribe )
        .returns( ui::e_confirm::yes );
    expect_combat();
    W.expect_some_animation();
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.units().exists( combat.defender.id ) );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 10 );
    REQUIRE( attacker.movement_points() == 0 );
  }

  SECTION( "soldier->brave, attacker wins" ) {
    combat = {
      .winner   = e_combat_winner::attacker,
      .attacker = { .outcome =
                        EuroUnitCombatOutcome::no_change{} },
      .defender = { .outcome =
                        NativeUnitCombatOutcome::destroyed{
                          .tribe_retains_horses  = true,
                          .tribe_retains_muskets = true } } };
    tie( combat.attacker.id, combat.defender.id ) =
        W.add_pair( e_unit_type::soldier,
                    e_native_unit_type::mounted_warrior );
    expect_combat();
    W.expect_some_animation();
    REQUIRE( tribe.muskets == 0 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 0 );
    REQUIRE( f() == expected );
    REQUIRE( tribe.muskets == 1 );
    REQUIRE( tribe.horse_herds == 0 );
    REQUIRE( tribe.horse_breeding == 25 );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.units().exists( combat.defender.id ) );
    REQUIRE( attacker.type() == e_unit_type::soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 10 );
    REQUIRE( attacker.movement_points() == 0 );
  }

  SECTION( "soldier->brave, attacker loses, no brave change" ) {
    combat = {
      .winner   = e_combat_winner::defender,
      .attacker = { .outcome =
                        EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } },
      .defender = { .outcome =
                        NativeUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_native_unit_type::brave );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_contains( W.kAttackingPlayer, "English",
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 10 );
    REQUIRE( attacker.movement_points() == 0 );
  }

  SECTION( "soldier->brave, attacker loses" ) {
    combat = {
      .winner   = e_combat_winner::defender,
      .attacker = { .outcome =
                        EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } },
      .defender = {
        .outcome = NativeUnitCombatOutcome::promoted{
          .to = e_native_unit_type::armed_brave } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::soldier, e_native_unit_type::brave );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_contains( W.kAttackingPlayer, "routed" );
    W.expect_msg_contains( W.kAttackingPlayer, "Brave",
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 10 );
    REQUIRE( attacker.movement_points() == 0 );
  }
}

TEST_CASE( "[attack-handlers] attack_dwelling_handler" ) {
  World W;
  W.create_land_9x9();
  CommandHandlerRunResult expected = { .order_was_run = true };
  CombatEuroAttackDwelling combat;
  Player& player = W.player( W.kAttackingPlayer );

  Tribe& tribe = W.tribe( W.kNativeTribe );
  TribeRelationship& relationship =
      tribe.relationship[W.kAttackingPlayer];
  relationship.player_has_attacked_tribe = true;
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
      .winner          = e_combat_winner::defender,
      .missions_burned = false,

      .attacker = { .outcome =
                        EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } },

      .defender = {
        .id      = dwelling.id,
        .outcome = DwellingCombatOutcome::no_change{} } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_contains( W.kAttackingPlayer, "routed" );
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 10 );
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
      .winner          = e_combat_winner::defender,
      .missions_burned = true,

      .attacker = { .outcome =
                        EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } },

      .defender = {
        .id      = dwelling.id,
        .outcome = DwellingCombatOutcome::no_change{} } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_equals(
        W.kAttackingPlayer,
        "The [Apache] revolt against [English] missions! All "
        "English missionaries eliminated!" );
    W.expect_msg_contains( W.kAttackingPlayer, "routed" );
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 10 );
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
      "attacker loses, missions burned, missionary "
      "eliminated, multiple dwellings with missionaries" ) {
    point const kDwellingCoord1     = W.kLandDefend;
    point constexpr kDwellingCoord2 = { .x = 1, .y = 5 };
    point constexpr kDwellingCoord3 = { .x = 5, .y = 8 };
    point constexpr kDwellingCoord4 = { .x = 4, .y = 3 };
    point constexpr kDwellingCoord5 = { .x = 8, .y = 2 };
    point constexpr kDwellingCoord6 = { .x = 7, .y = 8 };
    Dwelling& dwelling2 =
        W.add_dwelling( kDwellingCoord2, W.kNativeTribe );
    Dwelling& dwelling3 =
        W.add_dwelling( kDwellingCoord3, W.kNativeTribe );
    Dwelling& dwelling4 =
        W.add_dwelling( kDwellingCoord4, W.kNativeTribe );
    Dwelling& dwelling5 =
        W.add_dwelling( kDwellingCoord5, W.kNativeTribe );
    Dwelling& dwelling6 =
        W.add_dwelling( kDwellingCoord6, e_tribe::cherokee );
    UnitId const missionary_id_1 =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling_id )
            .id();
    UnitId const missionary_id_2 =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling2.id )
            .id();
    UnitId const missionary_id_3 =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling3.id )
            .id();
    UnitId const missionary_id_4 =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling4.id )
            .id();
    /* omit 5 */
    UnitId const missionary_id_6 =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling6.id )
            .id();
    BASE_CHECK( W.kNativeTribe != e_tribe::cherokee );

    VisibilityForPlayer const viz( W.ss(), player.type );

    W.map_updater().make_squares_visible(
        W.kAttackingPlayer,
        { kDwellingCoord1, kDwellingCoord2, kDwellingCoord3,
          kDwellingCoord4, kDwellingCoord5, kDwellingCoord6 } );
    W.map_updater().make_squares_fogged(
        W.kAttackingPlayer,
        { kDwellingCoord1, kDwellingCoord2, kDwellingCoord3,
          kDwellingCoord4, kDwellingCoord5, kDwellingCoord6 } );
    // This guy will keep dwelling 3 in the clear.
    W.add_unit_on_map( e_unit_type::free_colonist,
                       { .x = 4, .y = 8 }, player.type );

    REQUIRE( viz.visible( kDwellingCoord1 ) ==
             e_tile_visibility::fogged );
    REQUIRE( viz.visible( kDwellingCoord2 ) ==
             e_tile_visibility::fogged );
    REQUIRE( viz.visible( kDwellingCoord3 ) ==
             e_tile_visibility::clear );
    REQUIRE( viz.visible( kDwellingCoord4 ) ==
             e_tile_visibility::fogged );
    REQUIRE( viz.visible( kDwellingCoord5 ) ==
             e_tile_visibility::fogged );
    REQUIRE( viz.visible( kDwellingCoord6 ) ==
             e_tile_visibility::fogged );

    REQUIRE( viz.dwelling_at( kDwellingCoord1 )
                 .maybe_member( &Dwelling::frozen )
                 .maybe_member( &FrozenDwelling::mission )
                 .has_value() );
    REQUIRE( viz.dwelling_at( kDwellingCoord2 )
                 .maybe_member( &Dwelling::frozen )
                 .maybe_member( &FrozenDwelling::mission )
                 .has_value() );
    REQUIRE( !viz.dwelling_at( kDwellingCoord3 )
                  .maybe_member( &Dwelling::frozen )
                  .maybe_member( &FrozenDwelling::mission )
                  .has_value() );
    REQUIRE( viz.dwelling_at( kDwellingCoord4 )
                 .maybe_member( &Dwelling::frozen )
                 .maybe_member( &FrozenDwelling::mission )
                 .has_value() );
    REQUIRE( !viz.dwelling_at( kDwellingCoord5 )
                  .maybe_member( &Dwelling::frozen )
                  .maybe_member( &FrozenDwelling::mission )
                  .has_value() );
    REQUIRE( viz.dwelling_at( kDwellingCoord6 )
                 .maybe_member( &Dwelling::frozen )
                 .maybe_member( &FrozenDwelling::mission )
                 .has_value() );

    combat = {
      .winner          = e_combat_winner::defender,
      .missions_burned = true,

      .attacker = { .outcome =
                        EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } },

      .defender = {
        .id      = dwelling.id,
        .outcome = DwellingCombatOutcome::no_change{} } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_equals(
        W.kAttackingPlayer,
        "The [Apache] revolt against [English] missions! All "
        "English missionaries eliminated!" );
    W.expect_msg_contains( W.kAttackingPlayer, "routed" );
    W.expect_msg_contains( W.kNativeTribe, "routed" );
    expected = { .order_was_run = true };
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( W.natives().dwelling_exists( dwelling2.id ) );
    REQUIRE( W.natives().dwelling_exists( dwelling3.id ) );
    REQUIRE( W.natives().dwelling_exists( dwelling4.id ) );
    REQUIRE( W.natives().dwelling_exists( dwelling5.id ) );
    REQUIRE( W.natives().dwelling_exists( dwelling6.id ) );
    REQUIRE( attacker.type() == e_unit_type::free_colonist );
    REQUIRE( dwelling.population == 5 );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( W.natives().coord_for( dwelling.id ) ==
             W.kLandDefend );
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 10 );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( !W.units().exists( missionary_id_1 ) );
    REQUIRE( !W.units().exists( missionary_id_2 ) );
    REQUIRE( !W.units().exists( missionary_id_3 ) );
    REQUIRE( !W.units().exists( missionary_id_4 ) );
    REQUIRE( W.units().exists( missionary_id_6 ) );
    REQUIRE( player.score_stats.dwellings_burned == 0 );
    REQUIRE( W.square( W.kLandDefend ).road == true );
    REQUIRE( W.units().exists( brave_id ) );
    REQUIRE( W.units().coord_for( brave_id ) ==
             Coord{ .x = 2, .y = 2 } );
    REQUIRE( W.natives().tribe_exists( W.kNativeTribe ) );

    // Clear because of attacker next to it.
    REQUIRE( viz.visible( kDwellingCoord1 ) ==
             e_tile_visibility::clear );
    // Clear because of removed mission.
    REQUIRE( viz.visible( kDwellingCoord2 ) ==
             e_tile_visibility::clear );
    // Clear because of free colonist next to it.
    REQUIRE( viz.visible( kDwellingCoord3 ) ==
             e_tile_visibility::clear );
    // Clear because of removed mission.
    REQUIRE( viz.visible( kDwellingCoord4 ) ==
             e_tile_visibility::clear );
    // No mission here.
    REQUIRE( viz.visible( kDwellingCoord5 ) ==
             e_tile_visibility::fogged );
    // Mission here, but different tribe.
    REQUIRE( viz.visible( kDwellingCoord6 ) ==
             e_tile_visibility::fogged );

    REQUIRE( !viz.dwelling_at( kDwellingCoord1 )
                  .maybe_member( &Dwelling::frozen )
                  .has_value() );
    REQUIRE( !viz.dwelling_at( kDwellingCoord2 )
                  .maybe_member( &Dwelling::frozen )
                  .has_value() );
    REQUIRE( !viz.dwelling_at( kDwellingCoord3 )
                  .maybe_member( &Dwelling::frozen )
                  .has_value() );
    REQUIRE( !viz.dwelling_at( kDwellingCoord4 )
                  .maybe_member( &Dwelling::frozen )
                  .has_value() );
    REQUIRE( viz.dwelling_at( kDwellingCoord5 )
                 .maybe_member( &Dwelling::frozen )
                 .has_value() );
    REQUIRE( viz.dwelling_at( kDwellingCoord6 )
                 .maybe_member( &Dwelling::frozen )
                 .has_value() );

    REQUIRE( !viz.dwelling_at( kDwellingCoord1 )
                  .maybe_member( &Dwelling::frozen )
                  .maybe_member( &FrozenDwelling::mission )
                  .has_value() );
    REQUIRE( !viz.dwelling_at( kDwellingCoord2 )
                  .maybe_member( &Dwelling::frozen )
                  .maybe_member( &FrozenDwelling::mission )
                  .has_value() );
    REQUIRE( !viz.dwelling_at( kDwellingCoord3 )
                  .maybe_member( &Dwelling::frozen )
                  .maybe_member( &FrozenDwelling::mission )
                  .has_value() );
    REQUIRE( !viz.dwelling_at( kDwellingCoord4 )
                  .maybe_member( &Dwelling::frozen )
                  .maybe_member( &FrozenDwelling::mission )
                  .has_value() );
    REQUIRE( !viz.dwelling_at( kDwellingCoord5 )
                  .maybe_member( &Dwelling::frozen )
                  .maybe_member( &FrozenDwelling::mission )
                  .has_value() );
    REQUIRE( viz.dwelling_at( kDwellingCoord6 )
                 .maybe_member( &Dwelling::frozen )
                 .maybe_member( &FrozenDwelling::mission )
                 .has_value() );
  }

  SECTION(
      "attacker loses, missions burned, missionary "
      "eliminated, post-declaration" ) {
    Player& player           = W.default_player();
    player.revolution.status = e_revolution_status::declared;
    UnitId const missionary_id =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling_id )
            .id();
    combat = {
      .winner          = e_combat_winner::defender,
      .missions_burned = true,

      .attacker = { .outcome =
                        EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } },

      .defender = {
        .id      = dwelling.id,
        .outcome = DwellingCombatOutcome::no_change{} } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_equals(
        W.kAttackingPlayer,
        "The [Apache] revolt against [Rebel] missions! All "
        "Rebel missionaries eliminated!" );
    W.expect_msg_contains( W.kAttackingPlayer, "routed" );
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 10 );
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
                                      W.kDefendingPlayer )
            .id();
    combat = {
      .winner          = e_combat_winner::defender,
      .missions_burned = true,

      .attacker = { .outcome =
                        EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } },

      .defender = {
        .id      = dwelling.id,
        .outcome = DwellingCombatOutcome::no_change{} } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_msg_equals(
        W.kAttackingPlayer,
        "The [Apache] revolt against [English] missions! All "
        "English missionaries eliminated!" );
    W.expect_msg_contains( W.kAttackingPlayer, "routed" );
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 10 );
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
      .winner          = e_combat_winner::attacker,
      .missions_burned = false,
      .attacker        = { .outcome =
                               EuroUnitCombatOutcome::no_change{} },
      .defender        = {
               .id      = dwelling.id,
               .outcome = DwellingCombatOutcome::population_decrease{
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 10 );
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
      .winner          = e_combat_winner::attacker,
      .missions_burned = false,
      .attacker        = { .outcome =
                               EuroUnitCombatOutcome::no_change{} },
      .defender        = {
               .id      = dwelling.id,
               .outcome = DwellingCombatOutcome::population_decrease{
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 10 );
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
      .winner          = e_combat_winner::attacker,
      .missions_burned = false,
      .attacker        = { .outcome =
                               EuroUnitCombatOutcome::promoted{
                                 .to = e_unit_type::veteran_soldier } },
      .defender        = {
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
    W.expect_promotion( W.kAttackingPlayer );
    W.expect_msg_equals(
        W.kAttackingPlayer,
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( player.score_stats.dwellings_burned == 1 );
    REQUIRE( W.square( W.kLandDefend ).road == false );
    REQUIRE( !W.units().exists( brave_id ) );
    REQUIRE_FALSE( W.natives().tribe_exists( W.kNativeTribe ) );
  }

  SECTION( "attacker wins, dwelling burned, post-declaration" ) {
    player.revolution.status = e_revolution_status::declared;
    dwelling.population      = 1;

    combat = {
      .winner          = e_combat_winner::attacker,
      .missions_burned = false,
      .attacker        = { .outcome =
                               EuroUnitCombatOutcome::promoted{
                                 .to = e_unit_type::veteran_soldier } },
      .defender        = {
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
    W.expect_promotion( W.kAttackingPlayer );
    W.expect_msg_equals(
        W.kAttackingPlayer,
        "[Apache] camp burned by the [Rebels]!" );
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
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
      .winner          = e_combat_winner::attacker,
      .missions_burned = false,
      .attacker        = { .outcome =
                               EuroUnitCombatOutcome::promoted{
                                 .to = e_unit_type::veteran_soldier } },
      .defender        = {
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
    W.expect_promotion( W.kAttackingPlayer );
    W.expect_msg_equals(
        W.kAttackingPlayer,
        "[Apache] capital burned by the [English]!" );
    W.expect_msg_equals(
        W.kAttackingPlayer,
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( player.score_stats.dwellings_burned == 1 );
    REQUIRE( W.square( W.kLandDefend ).road == false );
    REQUIRE( !W.units().exists( brave_id ) );
    REQUIRE( W.natives().tribe_exists( W.kNativeTribe ) );
  }

  SECTION(
      "attacker wins, dwelling burned, tribe not wiped out, "
      "capital burned, post-declaration" ) {
    dwelling.population      = 1;
    dwelling.is_capital      = true;
    player.revolution.status = e_revolution_status::declared;

    combat = {
      .winner          = e_combat_winner::attacker,
      .missions_burned = false,
      .attacker        = { .outcome =
                               EuroUnitCombatOutcome::promoted{
                                 .to = e_unit_type::veteran_soldier } },
      .defender        = {
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
    W.expect_promotion( W.kAttackingPlayer );
    W.expect_msg_equals(
        W.kAttackingPlayer,
        "[Apache] capital burned by the [Rebels]!" );
    W.expect_msg_equals(
        W.kAttackingPlayer,
        "The [Apache] bow before the might of the [Rebels]!" );

    expected = { .order_was_run       = true,
                 .units_to_prioritize = {} };
    REQUIRE( f() == expected );

    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( attacker.type() == e_unit_type::veteran_soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
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
      .winner          = e_combat_winner::attacker,
      .missions_burned = false,
      .attacker        = { .outcome =
                               EuroUnitCombatOutcome::promoted{
                                 .to = e_unit_type::veteran_soldier } },
      .defender        = {
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
    W.expect_promotion( W.kAttackingPlayer );
    W.expect_convert();
    W.expect_msg_equals(
        W.kAttackingPlayer,
        fmt::format( "[Apache] camp burned by the [English]! "
                     "[Missionary] flees in panic! Treasure "
                     "worth [123\x7f] has been recovered! It "
                     "will take a [Galleon] to transport this "
                     "treasure back to [London]." ) );
    W.expect_some_animation(); // treasure enpixelation.
    W.expect_tribe_wiped_out( "Apache" );
    UnitId const expected_convert_id  = UnitId{ 5 };
    UnitId const expected_treasure_id = UnitId{ 6 };

    expected = { .order_was_run       = true,
                 .units_to_prioritize = {
                   expected_convert_id, expected_treasure_id } };
    REQUIRE( f() == expected );

    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    REQUIRE_FALSE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( attacker.type() == e_unit_type::veteran_soldier );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kLandAttack );
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
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
                                      W.kDefendingPlayer )
            .id();
    dwelling.population = 1;
    dwelling.is_capital = true;

    combat = {
      .winner          = e_combat_winner::attacker,
      .missions_burned = false,
      .attacker        = { .outcome =
                               EuroUnitCombatOutcome::promoted{
                                 .to = e_unit_type::veteran_soldier } },
      .defender        = {
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
    W.expect_promotion( W.kAttackingPlayer );
    W.expect_msg_equals(
        W.kAttackingPlayer,
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( !W.units().exists( missionary_id ) );
    REQUIRE( player.score_stats.dwellings_burned == 1 );
    REQUIRE( W.square( W.kLandDefend ).road == false );
    REQUIRE( !W.units().exists( brave_id ) );
    REQUIRE_FALSE( W.natives().tribe_exists( W.kNativeTribe ) );
  }

  SECTION(
      "attacker wins, population decrease, with convert, "
      "post-declaration" ) {
    Player& attacking_player = W.player( W.kAttackingPlayer );
    attacking_player.revolution.status =
        e_revolution_status::declared;
    UnitId const missionary_id =
        W.add_missionary_in_dwelling( e_unit_type::missionary,
                                      dwelling_id )
            .id();
    combat = {
      .winner          = e_combat_winner::attacker,
      .missions_burned = false,
      .attacker        = { .outcome =
                               EuroUnitCombatOutcome::no_change{} },
      .defender        = {
               .id      = dwelling.id,
               .outcome = DwellingCombatOutcome::population_decrease{
                 .convert_produced = true } } };
    combat.attacker.id = W.add_attacker( e_unit_type::soldier );
    expect_combat();
    W.expect_some_animation();
    W.expect_rebel_convert();
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( relationship.tribal_alarm == 10 );
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
}

TEST_CASE(
    "[attack-handlers] attack_dwelling_handler animation "
    "visibility" ) {
  World W;
  W.create_land_9x9();
  CommandHandlerRunResult expected = { .order_was_run = true };
  CombatEuroAttackDwelling combat;
  Tribe& tribe = W.tribe( W.kNativeTribe );
  TribeRelationship& relationship =
      tribe.relationship[W.kAttackingPlayer];
  relationship.player_has_attacked_tribe = true;
  REQUIRE( relationship.tribal_alarm == 0 );
  Dwelling& dwelling =
      W.add_dwelling( W.kLandDefend, W.kNativeTribe );
  // We need at least one of these for the animation to show.
  W.settings().in_game_options.game_menu_options
      [e_game_menu_option::show_foreign_moves] = true;
  W.settings()
      .in_game_options
      .game_menu_options[e_game_menu_option::show_indian_moves] =
      true;
  // Make kThirdPlayer the viewer.
  auto const kViewingPlayer = W.kThirdPlayer;
  W.land_view().map_revealed =
      MapRevealed::player{ .type = kViewingPlayer };
  // This one is needed for the kViewingPlayer to see the anima-
  // tion at all.
  W.map_updater().make_squares_visible( kViewingPlayer,
                                        { W.kLandAttack } );
  // Make the dwelling square fogged so that we can test that it
  // becomes visible, which it will because the kViewingPlayer
  // sees the animation.
  W.map_updater().make_squares_visible( kViewingPlayer,
                                        { W.kLandDefend } );
  W.map_updater().make_squares_fogged( kViewingPlayer,
                                       { W.kLandDefend } );
  VisibilityForPlayer const viz( W.ss(), kViewingPlayer );
  BASE_CHECK( viz.visible( W.kLandDefend ) ==
              e_tile_visibility::fogged );
  NativeUnitId const brave_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 2, .y = 2 }, dwelling.id )
          .id;
  W.square( W.kLandDefend ).road = true;
  dwelling.population            = 5;
  // Use only the id after the dwelling is destroyed.
  DwellingId const dwelling_id = dwelling.id;

  auto const expect_combat = [&] {
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

  dwelling.population = 1;

  combat = {
    .winner          = e_combat_winner::attacker,
    .missions_burned = false,
    .attacker        = { .outcome =
                             EuroUnitCombatOutcome::promoted{
                               .to = e_unit_type::veteran_soldier } },
    .defender        = { .id      = dwelling.id,
                         .outcome = DwellingCombatOutcome::destruction{
                           .braves_to_kill        = {},
                           .missionary_to_release = {},
                           .treasure_amount       = {},
                           .tribe_destroyed       = e_tribe::apache,
                           .convert_produced      = false } } };
  combat.attacker.id = W.add_attacker( e_unit_type::soldier );
  expect_combat();
  W.expect_some_animation();
  W.expect_promotion( W.kAttackingPlayer );
  W.expect_msg_equals(
      W.kAttackingPlayer,
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
  REQUIRE( attacker.player_type() == W.kAttackingPlayer );
  REQUIRE( attacker.movement_points() == 0 );
  REQUIRE( W.player( W.kAttackingPlayer )
               .score_stats.dwellings_burned == 1 );
  REQUIRE( W.square( W.kLandDefend ).road == false );
  REQUIRE( !W.units().exists( brave_id ) );
  REQUIRE_FALSE( W.natives().tribe_exists( W.kNativeTribe ) );
  REQUIRE( viz.visible( W.kLandDefend ) ==
           e_tile_visibility::clear );
}

TEST_CASE( "[attack-handlers] naval_battle_handler" ) {
  World W;
  W.create_default_map();
  CombatShipAttackShip combat;
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
      .winner = nothing,
      .attacker =
          { .outcome = EuroNavalUnitCombatOutcome::no_change{} },
      .defender = {
        .outcome = EuroNavalUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    expect_combat();
    W.expect_some_animation();
    W.expect_some_animation();
    W.expect_evaded( W.kAttackingPlayer );
    W.expect_evaded( W.kDefendingPlayer );
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( defender.player_type() == W.kDefendingPlayer );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( !attacker.orders().holds<unit_orders::damaged>() );
    REQUIRE( !defender.orders().holds<unit_orders::damaged>() );
    REQUIRE( attacker.cargo().count_items() == 0 );
    REQUIRE( defender.cargo().count_items() == 0 );
  }

  SECTION( "defender damaged, sent to harbor" ) {
    combat = {
      .winner = e_combat_winner::attacker,
      .attacker =
          { .outcome = EuroNavalUnitCombatOutcome::no_change{} },
      .defender = {
        .outcome = EuroNavalUnitCombatOutcome::damaged{
          .port = ShipRepairPort::european_harbor{} } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    expect_combat();
    W.expect_some_animation();
    W.expect_some_animation();
    REQUIRE( W.units()
                 .unit_for( combat.attacker.id )
                 .movement_points() == 8 );
    W.expect_msg_contains( W.kAttackingPlayer, "damaged",
                           "La Rochelle", "repair" );
    W.expect_msg_contains( W.kDefendingPlayer, "damaged",
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
          .port_status = PortStatus::in_port{},
          .sailed_from = nothing } );
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( defender.player_type() == W.kDefendingPlayer );
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
        W.add_colony( { .x = 2, .y = 2 }, W.kDefendingPlayer );
    combat = {
      .winner   = e_combat_winner::attacker,
      .attacker = { .outcome =
                        EuroNavalUnitCombatOutcome::moved{
                          .to = W.kWaterDefend } },
      .defender = {
        .outcome = EuroNavalUnitCombatOutcome::damaged{
          .port =
              ShipRepairPort::colony{ .id = colony.id } } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    expect_combat();
    W.expect_some_animation();
    W.expect_some_animation();
    W.expect_msg_contains( W.kAttackingPlayer, "Merchantman",
                           "damaged", "1" );
    W.expect_msg_contains( W.kDefendingPlayer, "Merchantman",
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( defender.player_type() == W.kDefendingPlayer );
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
        W.add_colony( { .x = 2, .y = 2 }, W.kDefendingPlayer );
    combat = {
      .winner   = e_combat_winner::attacker,
      .attacker = { .outcome =
                        EuroNavalUnitCombatOutcome::moved{
                          .to = W.kWaterDefend } },
      .defender = {
        .outcome = EuroNavalUnitCombatOutcome::damaged{
          .port =
              ShipRepairPort::colony{ .id = colony.id } } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::caravel );
    expect_combat();
    W.expect_some_animation();
    W.expect_some_animation();
    W.expect_msg_contains( W.kAttackingPlayer, "Caravel",
                           "damaged", "1" );
    W.expect_msg_contains( W.kDefendingPlayer, "Caravel",
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
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( defender.player_type() == W.kDefendingPlayer );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 4 );
    REQUIRE( !attacker.orders().holds<unit_orders::damaged>() );
    REQUIRE( defender.orders() ==
             unit_orders{ unit_orders::none{} } );
    REQUIRE( attacker.cargo().count_items() == 0 );
    REQUIRE( defender.cargo().count_items() == 0 );
  }

  SECTION( "defender sunk with affected units" ) {
    UnitId const merchantman_id =
        W.add_defender( e_unit_type::merchantman );
    UnitId const privateer_id =
        W.add_defender( e_unit_type::privateer );
    UnitId const galleon_id =
        W.add_defender( e_unit_type::galleon );
    W.add_commodity_in_cargo( e_commodity::silver, galleon_id );
    REQUIRE(
        W.units().unit_for( galleon_id ).cargo().count_items() ==
        1 );
    combat = {
      .winner = e_combat_winner::attacker,
      .attacker =
          { .outcome = EuroNavalUnitCombatOutcome::no_change{} },
      .defender                = { .outcome =
                                       EuroNavalUnitCombatOutcome::sunk{} },
      .affected_defender_units = {
        { merchantman_id,
          AffectedNavalDefender{
            .id      = merchantman_id,
            .outcome = EuroNavalUnitCombatOutcome::sunk{} } },
        { galleon_id,
          AffectedNavalDefender{
            .id = galleon_id,
            .outcome =
                EuroNavalUnitCombatOutcome::damaged{
                  .port =
                      ShipRepairPort::european_harbor{} } } },
        { privateer_id,
          AffectedNavalDefender{
            .id = privateer_id,
            .outcome =
                EuroNavalUnitCombatOutcome::sunk{} } } } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    expect_combat();
    W.expect_some_animation();
    W.expect_some_animation();
    W.expect_unit_captures_cargo(
        /*src=*/galleon_id,
        /*dst=*/combat.attacker.id,
        CapturableCargo{
          .items    = { .commodities = { { .type =
                                               e_commodity::silver,
                                           .quantity = 100 } } },
          .max_take = 2 } );
    W.euro_agent( W.kDefendingPlayer )
        .EXPECT__notify_captured_cargo(
            W.expect_defending_player(),
            W.expect_attacking_player(),
            W.expect_unit_of_type( e_unit_type::privateer ),
            Commodity{ .type     = e_commodity::silver,
                       .quantity = 100 } );
    REQUIRE( W.units()
                 .unit_for( combat.attacker.id )
                 .movement_points() == 8 );
    {
      auto A = W.kAttackingPlayer;
      auto D = W.kDefendingPlayer;
      // These go in order of IDs.
      W.expect_msg_contains( A, "sunk" );    // defender
      W.expect_msg_contains( A, "sunk" );    // merchantman
      W.expect_msg_contains( A, "sunk" );    // privateer
      W.expect_msg_contains( A, "damaged" ); // galleon
      W.expect_msg_contains( D, "sunk" );    // defender
      W.expect_msg_contains( D, "sunk" );    // merchantman
      W.expect_msg_contains( D, "sunk" );    // privateer
      W.expect_msg_contains( D, "damaged" ); // galleon
    }
    REQUIRE( f() == expected );
    Unit const& attacker =
        W.units().unit_for( combat.attacker.id );
    Unit const& galleon = W.units().unit_for( galleon_id );
    REQUIRE( !W.units().exists( combat.defender.id ) );
    REQUIRE( !W.units().exists( merchantman_id ) );
    REQUIRE( !W.units().exists( privateer_id ) );
    REQUIRE( W.units().coord_for( attacker.id() ) ==
             W.kWaterAttack );
    REQUIRE(
        as_const( W.units() ).ownership_of( galleon.id() ) ==
        UnitOwnership::harbor{
          .port_status = PortStatus::in_port{},
          .sailed_from = nothing } );
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( galleon.player_type() == W.kDefendingPlayer );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( galleon.movement_points() == 6 );
    REQUIRE( !attacker.orders().holds<unit_orders::damaged>() );
    REQUIRE( galleon.orders() ==
             unit_orders{ unit_orders::damaged{
               .turns_until_repair = 10 } } );
    REQUIRE( attacker.cargo().count_items() == 1 );
    REQUIRE( attacker.cargo().commodities().size() == 1 );
    REQUIRE( attacker.cargo().commodities().at( 0 ) ==
             pair{ Commodity{ .type     = e_commodity::silver,
                              .quantity = 100 },
                   0 } );
    REQUIRE( galleon.cargo().count_items() == 0 );
  }

  SECTION( "attacker sunk" ) {
    combat = {
      .winner   = e_combat_winner::defender,
      .attacker = { .outcome =
                        EuroNavalUnitCombatOutcome::sunk{} },
      .defender = {
        .outcome = EuroNavalUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    expect_combat();
    W.expect_some_animation();
    W.expect_some_animation();
    W.expect_ship_sunk( W.kAttackingPlayer );
    W.expect_ship_sunk( W.kDefendingPlayer );
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
    REQUIRE( defender.player_type() == W.kDefendingPlayer );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( !defender.orders().holds<unit_orders::damaged>() );
  }

  SECTION( "attacker sunk containing units" ) {
    combat = {
      .winner   = e_combat_winner::defender,
      .attacker = { .outcome =
                        EuroNavalUnitCombatOutcome::sunk{} },
      .defender = {
        .outcome = EuroNavalUnitCombatOutcome::no_change{} } };
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
    W.expect_some_animation();
    W.expect_msg_contains( W.kAttackingPlayer, "sunk" );
    W.expect_msg_contains( W.kAttackingPlayer, "Two", "units",
                           "lost" );
    W.expect_ship_sunk( W.kDefendingPlayer );
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
    REQUIRE( defender.player_type() == W.kDefendingPlayer );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( !defender.orders().holds<unit_orders::damaged>() );
    REQUIRE( defender.cargo().count_items() == 0 );
  }

  SECTION( "attacker damaged with commodity cargo" ) {
    combat = {
      .winner = e_combat_winner::defender,
      .attacker =
          { .outcome =
                EuroNavalUnitCombatOutcome::damaged{
                  .port = ShipRepairPort::european_harbor{} } },
      .defender = {
        .outcome = EuroNavalUnitCombatOutcome::no_change{} } };
    tie( combat.attacker.id, combat.defender.id ) = W.add_pair(
        e_unit_type::privateer, e_unit_type::merchantman );
    Unit& attacker = W.units().unit_for( combat.attacker.id );
    Unit& defender = W.units().unit_for( combat.defender.id );
    add_commodity_to_cargo(
        W.units(),
        Commodity{ .type = e_commodity::ore, .quantity = 10 },
        attacker.cargo(), /*slot=*/0,
        /*try_other_slots=*/false );
    add_commodity_to_cargo(
        W.units(),
        Commodity{ .type = e_commodity::lumber, .quantity = 20 },
        attacker.cargo(), /*slot=*/1,
        /*try_other_slots=*/false );
    add_commodity_to_cargo(
        W.units(),
        Commodity{ .type = e_commodity::ore, .quantity = 10 },
        defender.cargo(), /*slot=*/0,
        /*try_other_slots=*/false );
    REQUIRE( attacker.cargo().count_items() == 2 );
    REQUIRE( defender.cargo().count_items() == 1 );
    expect_combat();
    W.expect_some_animation();
    W.expect_some_animation();
    W.expect_unit_captures_cargo(
        /*src=*/combat.attacker.id,
        /*dst=*/combat.defender.id,
        CapturableCargo{
          .items =
              {
                .commodities =
                    {
                      { .type     = e_commodity::lumber,
                        .quantity = 20 },
                      { .type     = e_commodity::ore,
                        .quantity = 10 },
                    },
              },
          .max_take = 3 } );
    W.euro_agent( W.kAttackingPlayer )
        .EXPECT__notify_captured_cargo(
            W.expect_attacking_player(),
            W.expect_defending_player(),
            W.expect_unit_of_type( e_unit_type::merchantman ),
            Commodity{ .type     = e_commodity::lumber,
                       .quantity = 20 } );
    W.euro_agent( W.kAttackingPlayer )
        .EXPECT__notify_captured_cargo(
            W.expect_attacking_player(),
            W.expect_defending_player(),
            W.expect_unit_of_type( e_unit_type::merchantman ),
            Commodity{ .type     = e_commodity::ore,
                       .quantity = 10 } );
    W.expect_msg_contains( W.kAttackingPlayer, "Privateer",
                           "damaged", "London" );
    W.expect_msg_contains( W.kDefendingPlayer, "Privateer",
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
          .port_status = PortStatus::in_port{},
          .sailed_from = nothing } );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kWaterDefend );
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( defender.player_type() == W.kDefendingPlayer );
    REQUIRE( attacker.movement_points() == 0 );
    REQUIRE( defender.movement_points() == 5 );
    REQUIRE( attacker.orders() ==
             unit_orders{ unit_orders::damaged{
               .turns_until_repair = 8 } } );
    REQUIRE( !defender.orders().holds<unit_orders::damaged>() );
    REQUIRE( attacker.cargo().count_items() == 0 );
    REQUIRE( defender.cargo().count_items() == 2 );
    REQUIRE( defender.cargo().commodities().size() == 2 );
    REQUIRE( defender.cargo().commodities().at( 0 ) ==
             pair{ Commodity{ .type     = e_commodity::ore,
                              .quantity = 20 },
                   0 } );
    REQUIRE( defender.cargo().commodities().at( 1 ) ==
             pair{ Commodity{ .type     = e_commodity::lumber,
                              .quantity = 20 },
                   1 } );
  }

  SECTION( "attacker damaged with unit cargo" ) {
    combat = {
      .winner = e_combat_winner::defender,
      .attacker =
          { .outcome =
                EuroNavalUnitCombatOutcome::damaged{
                  .port = ShipRepairPort::european_harbor{} } },
      .defender = {
        .outcome = EuroNavalUnitCombatOutcome::no_change{} } };
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
    W.expect_some_animation();
    W.expect_msg_contains( W.kAttackingPlayer, "Privateer",
                           "damaged", "London" );
    W.expect_msg_contains( W.kAttackingPlayer, "Two",
                           "onboard" );
    W.expect_msg_contains( W.kDefendingPlayer, "Privateer",
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
          .port_status = PortStatus::in_port{},
          .sailed_from = nothing } );
    REQUIRE( W.units().coord_for( defender.id() ) ==
             W.kWaterDefend );
    REQUIRE( attacker.player_type() == W.kAttackingPlayer );
    REQUIRE( defender.player_type() == W.kDefendingPlayer );
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

TEST_CASE(
    "[attack-handlers] attack_colony_undefended_handler" ) {
  World W;
  W.create_default_map();
  CombatEuroAttackUndefendedColony combat;
  // TODO
}

} // namespace
} // namespace rn
