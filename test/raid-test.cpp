/****************************************************************
**raid-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-10.
*
* Description: Unit tests for the raid module.
*
*****************************************************************/
// Under test.
#include "src/raid.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/icombat.hpp"
#include "test/mocks/ieuro-agent.hpp"
#include "test/mocks/inative-agent.hpp"
#include "test/mocks/irand.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/agents.hpp"
#include "src/plane-stack.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/native-unit.rds.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;
using ::mock::matchers::_;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_player::dutch );
    add_player( e_player::english );
    set_default_player_type( e_player::dutch );
    set_default_player_as_human();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, L, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[raid] raid_unit" ) {
  World W;
  MockLandViewPlane mock_land_view;
  W.planes().get().set_bottom<ILandViewPlane>( mock_land_view );

  CombatBraveAttackEuro combat;
  Coord const defender_coord{ .x = 1, .y = 0 };
  Coord const attacker_coord{ .x = 0, .y = 0 };
  MockINativeAgent& native_agent =
      W.native_agent( e_tribe::arawak );
  MockIEuroAgent& euro_agent =
      W.euro_agent( W.default_player_type() );

  auto [dwelling, brave] = W.add_dwelling_and_brave(
      attacker_coord, e_tribe::arawak );
  // This is for use after the brave is destroyed.
  NativeUnitId const brave_id = brave.id;

  auto f = [&] {
    co_await_test(
        raid_unit( W.ss(), W.ts(), brave, defender_coord ) );
  };

  SECTION( "brave, one euro, brave loses, soldier promoted" ) {
    Unit const& soldier = W.add_unit_on_map(
        e_unit_type::soldier, defender_coord );
    UnitId const soldier_id = soldier.id();

    combat = {
      .winner   = e_combat_winner::defender,
      .attacker = { .id              = brave.id,
                    .modifiers       = {},
                    .base_weight     = 1,
                    .modified_weight = 1,
                    .outcome =
                        NativeUnitCombatOutcome::destroyed{} },
      .defender = { .id              = soldier.id(),
                    .modifiers       = {},
                    .base_weight     = 2,
                    .modified_weight = 2,
                    .outcome = EuroUnitCombatOutcome::promoted{
                      .to = e_unit_type::veteran_soldier } } };
    W.combat()
        .EXPECT__brave_attack_euro( brave, soldier )
        .returns( combat );
    mock_land_view.EXPECT__ensure_visible( attacker_coord );
    mock_land_view.EXPECT__ensure_visible( defender_coord );
    euro_agent.EXPECT__message_box(
        "[Arawaks] make surprise raid! Terror descends upon "
        "colonists! Arawak chief unavailable for comment." );
    mock_land_view.EXPECT__animate( _ );
    euro_agent.EXPECT__message_box(
        "[Dutch] Soldier promoted to [Veteran Soldier] for "
        "victory in combat!" );
    native_agent.EXPECT__on_attack_unit_finished( combat );
    f();
    // Brave.
    REQUIRE( !W.units().exists( brave_id ) );
    // Defender(s).
    REQUIRE( W.units().exists( soldier_id ) );
    REQUIRE( soldier.movement_points() == 1 );
    REQUIRE( soldier.type() == e_unit_type::veteran_soldier );
    REQUIRE( W.units().coord_for( soldier_id ) ==
             defender_coord );
  }

  SECTION( "brave, one euro, brave wins" ) {
    Unit const& soldier = W.add_unit_on_map(
        e_unit_type::soldier, defender_coord );
    UnitId const soldier_id = soldier.id();

    combat = {
      .winner = e_combat_winner::attacker,
      .attacker =
          { .id              = brave.id,
            .modifiers       = {},
            .base_weight     = 1,
            .modified_weight = 1,
            .outcome =
                NativeUnitCombatOutcome::promoted{
                  .to = e_native_unit_type::armed_brave } },
      .defender = { .id              = soldier.id(),
                    .modifiers       = {},
                    .base_weight     = 2,
                    .modified_weight = 2,
                    .outcome = EuroUnitCombatOutcome::demoted{
                      .to = e_unit_type::free_colonist } } };
    W.combat()
        .EXPECT__brave_attack_euro( brave, soldier )
        .returns( combat );
    mock_land_view.EXPECT__ensure_visible( attacker_coord );
    mock_land_view.EXPECT__ensure_visible( defender_coord );
    euro_agent.EXPECT__message_box(
        "[Arawaks] make surprise raid! Terror descends upon "
        "colonists! Arawak chief unavailable for comment." );
    mock_land_view.EXPECT__animate( _ );
    euro_agent.EXPECT__message_box(
        "[Dutch] [Soldier] routed! Unit demoted to colonist "
        "status." );
    euro_agent.EXPECT__message_box(
        "[Arawak] Braves have acquired [muskets] upon victory "
        "in combat!" );
    native_agent.EXPECT__message_box(
        "[Arawak] Braves have acquired [muskets] upon victory "
        "in combat!" );
    native_agent.EXPECT__message_box(
        "[Dutch] [Soldier] routed! Unit demoted to colonist "
        "status." );
    native_agent.EXPECT__on_attack_unit_finished( combat );
    f();
    // Brave.
    REQUIRE( W.units().exists( brave_id ) );
    REQUIRE( brave.movement_points == 1 );
    REQUIRE( brave.type == e_native_unit_type::armed_brave );
    REQUIRE( W.units().coord_for( brave_id ) == attacker_coord );
    // Defender(s).
    REQUIRE( W.units().exists( soldier_id ) );
    REQUIRE( soldier.movement_points() == 1 );
    REQUIRE( soldier.type() == e_unit_type::free_colonist );
    REQUIRE( W.units().coord_for( soldier_id ) ==
             defender_coord );
  }

  SECTION( "brave, three euro, brave loses" ) {
    // The soldier should be chosen as defender.
    Unit const& free_colonist1 = W.add_unit_on_map(
        e_unit_type::free_colonist, defender_coord );
    UnitId const free_colonist1_id = free_colonist1.id();
    // Put the soldier in the middle so we can test that it gets
    // picked for the right reasons.
    Unit const& soldier = W.add_unit_on_map(
        e_unit_type::soldier, defender_coord );
    UnitId const soldier_id    = soldier.id();
    Unit const& free_colonist2 = W.add_unit_on_map(
        e_unit_type::free_colonist, defender_coord );
    UnitId const free_colonist2_id = free_colonist2.id();

    combat = {
      .winner   = e_combat_winner::defender,
      .attacker = { .id              = brave.id,
                    .modifiers       = {},
                    .base_weight     = 1,
                    .modified_weight = 1,
                    .outcome =
                        NativeUnitCombatOutcome::destroyed{} },
      .defender = {
        .id              = soldier.id(),
        .modifiers       = {},
        .base_weight     = 2,
        .modified_weight = 2,
        .outcome = EuroUnitCombatOutcome::no_change{} } };
    W.combat()
        .EXPECT__brave_attack_euro( brave, soldier )
        .returns( combat );
    mock_land_view.EXPECT__ensure_visible( attacker_coord );
    mock_land_view.EXPECT__ensure_visible( defender_coord );
    euro_agent.EXPECT__message_box(
        "[Arawaks] make surprise raid! Terror descends upon "
        "colonists! Arawak chief unavailable for comment." );
    mock_land_view.EXPECT__animate( _ );
    euro_agent.EXPECT__message_box(
        "[Dutch] Soldier defeats [Arawak] Brave in the "
        "wilderness!" );
    native_agent.EXPECT__on_attack_unit_finished( combat );
    f();
    // Brave.
    REQUIRE( !W.units().exists( brave_id ) );
    // Defender(s).
    REQUIRE( W.units().exists( soldier_id ) );
    REQUIRE( W.units().exists( free_colonist1_id ) );
    REQUIRE( W.units().exists( free_colonist2_id ) );
    REQUIRE( soldier.movement_points() == 1 );
    REQUIRE( free_colonist1.movement_points() == 1 );
    REQUIRE( free_colonist2.movement_points() == 1 );
    REQUIRE( soldier.type() == e_unit_type::soldier );
    REQUIRE( free_colonist1.type() ==
             e_unit_type::free_colonist );
    REQUIRE( free_colonist2.type() ==
             e_unit_type::free_colonist );
    REQUIRE( W.units().coord_for( soldier_id ) ==
             defender_coord );
    REQUIRE( W.units().coord_for( free_colonist1_id ) ==
             defender_coord );
    REQUIRE( W.units().coord_for( free_colonist2_id ) ==
             defender_coord );
  }

  // Here we do the same as above but create the units with the
  // non-default (and hence non-human) player, that way the
  // battle will not be viz'able and so the animation should be
  // suppressed.
  SECTION( "brave, three euro, brave loses, no anim" ) {
    // The soldier should be chosen as defender.
    Unit const& free_colonist1 =
        W.add_unit_on_map( e_unit_type::free_colonist,
                           defender_coord, e_player::english );
    UnitId const free_colonist1_id = free_colonist1.id();
    // Put the soldier in the middle so we can test that it gets
    // picked for the right reasons.
    Unit const& soldier =
        W.add_unit_on_map( e_unit_type::soldier, defender_coord,
                           e_player::english );
    UnitId const soldier_id = soldier.id();
    Unit const& free_colonist2 =
        W.add_unit_on_map( e_unit_type::free_colonist,
                           defender_coord, e_player::english );
    UnitId const free_colonist2_id = free_colonist2.id();

    combat = {
      .winner   = e_combat_winner::defender,
      .attacker = { .id              = brave.id,
                    .modifiers       = {},
                    .base_weight     = 1,
                    .modified_weight = 1,
                    .outcome =
                        NativeUnitCombatOutcome::destroyed{} },
      .defender = {
        .id              = soldier.id(),
        .modifiers       = {},
        .base_weight     = 2,
        .modified_weight = 2,
        .outcome = EuroUnitCombatOutcome::no_change{} } };
    W.combat()
        .EXPECT__brave_attack_euro( brave, soldier )
        .returns( combat );
    W.euro_agent( e_player::english )
        .EXPECT__message_box(
            "[Arawaks] make surprise raid! Terror descends upon "
            "colonists! Arawak chief unavailable for comment." );
    W.euro_agent( e_player::english )
        .EXPECT__message_box(
            "[English] Soldier defeats [Arawak] Brave in the "
            "wilderness!" );
    native_agent.EXPECT__on_attack_unit_finished( combat );
    f();
    // Brave.
    REQUIRE( !W.units().exists( brave_id ) );
    // Defender(s).
    REQUIRE( W.units().exists( soldier_id ) );
    REQUIRE( W.units().exists( free_colonist1_id ) );
    REQUIRE( W.units().exists( free_colonist2_id ) );
    REQUIRE( soldier.movement_points() == 1 );
    REQUIRE( free_colonist1.movement_points() == 1 );
    REQUIRE( free_colonist2.movement_points() == 1 );
    REQUIRE( soldier.type() == e_unit_type::soldier );
    REQUIRE( free_colonist1.type() ==
             e_unit_type::free_colonist );
    REQUIRE( free_colonist2.type() ==
             e_unit_type::free_colonist );
    REQUIRE( W.units().coord_for( soldier_id ) ==
             defender_coord );
    REQUIRE( W.units().coord_for( free_colonist1_id ) ==
             defender_coord );
    REQUIRE( W.units().coord_for( free_colonist2_id ) ==
             defender_coord );
  }
}

TEST_CASE( "[raid] raid_colony" ) {
  World W;
  MockLandViewPlane mock_land_view;
  W.planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  e_tribe const tribe_type   = e_tribe::arawak;
  Coord const attacker_coord = { .x = 0, .y = 0 };
  Coord const defender_coord = { .x = 1, .y = 0 };
  Dwelling const& dwelling =
      W.add_dwelling( attacker_coord, tribe_type );
  DwellingId const dwelling_id = dwelling.id;
  Colony& colony               = W.add_colony( defender_coord );
  Coord const colony_location  = colony.location;
  Unit const& worker =
      W.add_unit_indoors( colony.id, e_indoor_job::bells );
  MockIEuroAgent& mock_euro_agent =
      W.euro_agent( W.default_player_type() );
  MockINativeAgent& mock_native_agent =
      W.native_agent( tribe_type );
  Player& player = W.default_player();
  player.money   = 1000;

  // Sanity checks.
  REQUIRE( W.square( colony.location ).road );

  auto f = [&]( NativeUnit& attacker ) {
    co_await_test(
        raid_colony( W.ss(), W.ts(), attacker, colony ) );
  };

  SECTION( "brave->soldier, brave wins" ) {
    NativeUnit& attacker = W.add_native_unit_on_map(
        e_native_unit_type::brave, attacker_coord, dwelling.id );
    Unit const& defender = W.add_unit_on_map(
        e_unit_type::soldier, colony.location, colony.player );
    Unit const& caravel = W.add_unit_on_map(
        e_unit_type::caravel, colony.location, colony.player );
    Unit const& frigate = W.add_unit_on_map(
        e_unit_type::frigate, colony.location, colony.player );

    // We'll give this colony a dry-dock so that we can test that
    // the damaged ship stays in this colony.
    colony.buildings[e_colony_building::drydock] = true;

    // These are for after the raid when they may no longer exist
    // and thus the associated references might be dangling.
    NativeUnitId const attacker_id = attacker.id;
    ColonyId const colony_id       = colony.id;
    UnitId const worker_id         = worker.id();
    UnitId const defender_id       = defender.id();
    UnitId const caravel_id        = caravel.id();
    UnitId const frigate_id        = frigate.id();

    // Note that, in the OG, the brave attacking a colony is al-
    // ways destroyed after an attack on a colony, provided the
    // colony is not destroyed in the process.
    CombatBraveAttackColony const combat{
      .winner           = e_combat_winner::attacker,
      .colony_id        = colony.id,
      .colony_destroyed = false,
      .attacker         = { .id = attacker.id,
                            .outcome =
                                NativeUnitCombatOutcome::destroyed{} },
      .defender         = { .id      = defender_id,
                            .outcome = EuroUnitCombatOutcome::demoted{
                              .to = e_unit_type::free_colonist } } };
    W.combat()
        .EXPECT__brave_attack_colony( attacker, defender,
                                      colony )
        .returns( combat );
    mock_land_view.EXPECT__ensure_visible( attacker_coord );
    mock_land_view.EXPECT__ensure_visible( defender_coord );
    mock_euro_agent.EXPECT__message_box(
        "[Arawaks] make surprise raid of [1]! Terror "
        "descends upon colonists! Arawak chief unavailable "
        "for comment." );
    mock_land_view.EXPECT__animate( _ );
    // none:                 12
    // commodity_stolen:     30
    // money_stolen:         21
    // building_destroyed:   12
    // ship_in_port_damaged: 25
    W.rand()
        .EXPECT__between_ints( 0, 100 - 1 )
        .returns( 75 ); // ship in port damaged.
    W.rand()
        .EXPECT__between_ints( 0, 2 - 1 )
        .returns( 1 ); // frigate damaged.
    mock_euro_agent.EXPECT__message_box(
        "[Dutch] [Soldier] routed! Unit demoted to colonist "
        "status." );
    mock_native_agent.EXPECT__message_box(
        "[Dutch] [Soldier] routed! Unit demoted to colonist "
        "status." );
    mock_euro_agent.EXPECT__message_box(
        "[Dutch] [Frigate] damaged in battle! Ship sent to "
        "[1] for repairs." );
    mock_native_agent.EXPECT__on_attack_colony_finished(
        combat, BraveAttackColonyEffect::ship_in_port_damaged{
                  .which   = frigate_id,
                  .sent_to = ShipRepairPort::colony{
                    .id = colony.id } } );
    f( attacker );

    REQUIRE( player.money == 1000 );
    REQUIRE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE_FALSE( W.units().exists( attacker_id ) );
    REQUIRE( W.colonies().exists( colony_id ) );
    REQUIRE( W.units().exists( defender_id ) );
    REQUIRE( defender.type() == e_unit_type::free_colonist );
    REQUIRE( defender.movement_points() == 1 );
    REQUIRE( W.units().exists( worker_id ) );
    REQUIRE( W.square( colony_location ).road );

    REQUIRE( W.units().exists( caravel_id ) );
    REQUIRE( W.units().exists( frigate_id ) );
    REQUIRE( W.units()
                 .unit_for( caravel_id )
                 .orders()
                 .holds<unit_orders::none>() );
    REQUIRE( W.units()
                 .unit_for( frigate_id )
                 .orders()
                 .holds<unit_orders::damaged>() );
    REQUIRE( as_const( W.units() ).ownership_of( caravel_id ) ==
             UnitOwnership::world{ .coord = colony_location } );
    // The frigate is being repaired in this colony, since it has
    // a drydock.
    REQUIRE( as_const( W.units() ).ownership_of( frigate_id ) ==
             UnitOwnership::world{ .coord = colony_location } );
  }

  SECTION( "brave->soldier, soldier on ship, brave loses" ) {
    NativeUnit& attacker = W.add_native_unit_on_map(
        e_native_unit_type::brave, attacker_coord, dwelling.id );
    Unit const& caravel = W.add_unit_on_map(
        e_unit_type::caravel, colony.location, colony.player );
    Unit const& defender = W.add_unit_in_cargo(
        e_unit_type::soldier, caravel.id() );

    colony.buildings[e_colony_building::newspaper] = true;

    // These are for after the raid when they may no longer exist
    // and thus the associated references might be dangling.
    NativeUnitId const attacker_id = attacker.id;
    ColonyId const colony_id       = colony.id;
    UnitId const worker_id         = worker.id();
    UnitId const defender_id       = defender.id();
    UnitId const caravel_id        = caravel.id();

    // Note that, in the OG, the brave attacking a colony is al-
    // ways destroyed after an attack on a colony, provided the
    // colony is not destroyed in the process.
    CombatBraveAttackColony const combat{
      .winner           = e_combat_winner::defender,
      .colony_id        = colony.id,
      .colony_destroyed = false,
      .attacker         = { .id = attacker.id,
                            .outcome =
                                NativeUnitCombatOutcome::destroyed{} },
      .defender         = { .id      = defender_id,
                            .outcome = EuroUnitCombatOutcome::promoted{
                              .to = e_unit_type::veteran_soldier } } };
    W.combat()
        .EXPECT__brave_attack_colony( attacker, defender,
                                      colony )
        .returns( combat );
    mock_land_view.EXPECT__ensure_visible( attacker_coord );
    mock_land_view.EXPECT__ensure_visible( defender_coord );
    mock_euro_agent.EXPECT__message_box(
        "[Arawaks] make surprise raid of [1]! Terror "
        "descends upon colonists! Arawak chief unavailable "
        "for comment." );
    mock_euro_agent.EXPECT__message_box(
        "Colonists on ships docked in [1] have been offboarded "
        "to help defend the colony!" );
    mock_land_view.EXPECT__animate( _ );
    // none:                 12
    // commodity_stolen:     30
    // money_stolen:         21
    // building_destroyed:   12
    // ship_in_port_damaged: 25
    W.rand()
        .EXPECT__between_ints( 0, 100 - 1 )
        .returns( 63 ); // building destroyed.
    W.rand()
        .EXPECT__between_ints( 0, 16 - 1 )
        .returns( 8 ); // "newspapers" slot.
    mock_euro_agent.EXPECT__message_box(
        "[Dutch] Soldier promoted to [Veteran Soldier] for "
        "victory in combat!" );
    mock_euro_agent.EXPECT__message_box(
        "[Arawak] raiding parties have destroyed the "
        "[Newspaper] in [1]!" );
    mock_native_agent.EXPECT__on_attack_colony_finished(
        combat, BraveAttackColonyEffect::building_destroyed{
                  .which = e_colony_building::newspaper } );
    f( attacker );

    REQUIRE( player.money == 1000 );
    REQUIRE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE_FALSE( W.units().exists( attacker_id ) );
    REQUIRE( W.colonies().exists( colony_id ) );
    REQUIRE_FALSE(
        colony.buildings[e_colony_building::newspaper] );
    REQUIRE( W.units().exists( defender_id ) );
    REQUIRE( defender.type() == e_unit_type::veteran_soldier );
    REQUIRE( defender.movement_points() == 1 );
    REQUIRE( defender.orders().holds<unit_orders::sentry>() );
    REQUIRE( as_const( W.units() ).ownership_of( defender_id ) ==
             UnitOwnership::world{ .coord = colony_location } );
    REQUIRE( W.units().exists( worker_id ) );
    REQUIRE( W.square( colony_location ).road );

    REQUIRE( W.units().exists( caravel_id ) );
    REQUIRE( caravel.cargo().slots_occupied() == 0 );
    REQUIRE( W.units()
                 .unit_for( caravel_id )
                 .orders()
                 .holds<unit_orders::none>() );
    REQUIRE( as_const( W.units() ).ownership_of( caravel_id ) ==
             UnitOwnership::world{ .coord = colony_location } );
  }

  SECTION( "brave->worker, brave loses" ) {
    NativeUnit& attacker = W.add_native_unit_on_map(
        e_native_unit_type::brave, attacker_coord, dwelling.id );
    Unit const& defender = worker;

    // These are for after the raid when they may no longer exist
    // and thus the associated references might be dangling.
    NativeUnitId const attacker_id = attacker.id;
    ColonyId const colony_id       = colony.id;
    UnitId const worker_id         = worker.id();
    UnitId const defender_id       = defender.id();

    // Note that, in the OG, the brave attacking a colony is al-
    // ways destroyed after an attack on a colony, provided the
    // colony is not destroyed in the process.
    CombatBraveAttackColony const combat{
      .winner           = e_combat_winner::defender,
      .colony_id        = colony.id,
      .colony_destroyed = false,
      .attacker         = { .id = attacker.id,
                            .outcome =
                                NativeUnitCombatOutcome::destroyed{} },
      .defender         = {
                .id      = defender_id,
                .outcome = EuroUnitCombatOutcome::no_change{} } };
    // Select which colony worker to be the defender. In this
    // case there is only one. This is done because there are not
    // military units.
    W.rand().EXPECT__between_ints( 0, 0 ).returns( 0 );
    W.combat()
        .EXPECT__brave_attack_colony( attacker, defender,
                                      colony )
        .returns( combat );
    mock_land_view.EXPECT__ensure_visible( attacker_coord );
    mock_land_view.EXPECT__ensure_visible( defender_coord );
    mock_euro_agent.EXPECT__message_box(
        "[Arawaks] make surprise raid of [1]! Terror "
        "descends upon colonists! Arawak chief unavailable "
        "for comment." );
    mock_land_view.EXPECT__animate( _ );
    // none:                 12
    // commodity_stolen:     30
    // money_stolen:         21
    // building_destroyed:   12
    // ship_in_port_damaged: 25
    W.rand()
        .EXPECT__between_ints( 0, 100 - 1 )
        .returns( 42 ); // money stolen.
    W.rand().EXPECT__between_ints( 30, 200 ).returns(
        123 ); // amount stolen.
    mock_euro_agent.EXPECT__message_box(
        "[Arawak] raiding party wiped out in [1]! Colonists "
        "celebrate!" );
    mock_euro_agent.EXPECT__message_box(
        "[Arawak] looting parties have stolen [123\x7f] from "
        "the treasury!" );
    mock_native_agent.EXPECT__on_attack_colony_finished(
        combat, BraveAttackColonyEffect::money_stolen{
                  .quantity = 123 } );
    f( attacker );

    REQUIRE( player.money == 877 );
    REQUIRE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE_FALSE( W.units().exists( attacker_id ) );
    REQUIRE( W.colonies().exists( colony_id ) );
    REQUIRE( W.units().exists( defender_id ) );
    REQUIRE( defender.movement_points() == 1 );
    REQUIRE( W.units().exists( worker_id ) );
    REQUIRE( W.square( colony_location ).road );
  }

  SECTION( "brave->worker, burn" ) {
    NativeUnit& attacker = W.add_native_unit_on_map(
        e_native_unit_type::brave, attacker_coord, dwelling.id );
    Unit const& defender = worker;

    // These are for after the raid when they may no longer exist
    // and thus the associated references might be dangling.
    NativeUnitId const attacker_id = attacker.id;
    ColonyId const colony_id       = colony.id;
    UnitId const worker_id         = worker.id();
    UnitId const defender_id       = defender.id();

    // Note that, in the OG, the brave attacking a colony is al-
    // ways destroyed after an attack on a colony, provided the
    // colony is not destroyed in the process. If it is destroyed
    // then the brave survives.
    CombatBraveAttackColony const combat{
      .winner           = e_combat_winner::attacker,
      .colony_id        = colony.id,
      .colony_destroyed = true,
      .attacker         = { .id = attacker.id,
                            .outcome =
                                NativeUnitCombatOutcome::no_change{} },
      .defender         = {
                .id      = defender_id,
                .outcome = EuroUnitCombatOutcome::destroyed{} } };
    // Select which colony worker to be the defender. In this
    // case there is only one. This is done because there are not
    // military units.
    W.rand().EXPECT__between_ints( 0, 0 ).returns( 0 );
    W.combat()
        .EXPECT__brave_attack_colony( attacker, defender,
                                      colony )
        .returns( combat );
    mock_land_view.EXPECT__ensure_visible( attacker_coord );
    mock_land_view.EXPECT__ensure_visible( defender_coord );
    mock_euro_agent.EXPECT__message_box(
        "[Arawaks] make surprise raid of [1]! Terror "
        "descends upon colonists! Arawak chief unavailable "
        "for comment." );
    mock_land_view.EXPECT__animate( _ );
    mock_euro_agent.EXPECT__show_woodcut(
        e_woodcut::colony_burning );
    mock_euro_agent.EXPECT__message_box(
        "[Arawak] massacre [Dutch] population in [1]! "
        "Colony set ablaze and decimated! The King demands "
        "accountability!" );
    mock_native_agent.EXPECT__on_attack_colony_finished(
        combat, BraveAttackColonyEffect::none{} );
    f( attacker );

    REQUIRE( player.money == 1000 );
    REQUIRE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( W.units().exists( attacker_id ) );
    REQUIRE_FALSE( W.colonies().exists( colony_id ) );
    REQUIRE_FALSE( W.units().exists( defender_id ) );
    REQUIRE_FALSE( W.units().exists( worker_id ) );
    REQUIRE_FALSE( W.square( colony_location ).road );
  }

  SECTION( "brave->worker, burn, with units at gate" ) {
    NativeUnit& attacker = W.add_native_unit_on_map(
        e_native_unit_type::brave, attacker_coord, dwelling.id );
    Unit& wagon_train =
        W.add_unit_on_map( e_unit_type::wagon_train,
                           colony.location, colony.player );
    Unit const& caravel = W.add_unit_on_map(
        e_unit_type::caravel, colony.location, colony.player );
    Unit const& frigate = W.add_unit_on_map(
        e_unit_type::frigate, colony.location, colony.player );
    Unit const& onboard = W.add_unit_in_cargo(
        e_unit_type::free_colonist, caravel.id() );

    // Non-combatant units at the gate will never be chosen to
    // defende, as in the OG. Only colony workers (assuming there
    // are no military units at the gate).
    Unit const& defender = worker;
    wagon_train.fortify();
    Unit& pioneer = W.add_unit_on_map(
        e_unit_type::pioneer, colony.location, colony.player );

    // These are for after the raid when they may no longer exist
    // and thus the associated references might be dangling.
    NativeUnitId const attacker_id = attacker.id;
    ColonyId const colony_id       = colony.id;
    UnitId const worker_id         = worker.id();
    UnitId const defender_id       = defender.id();
    UnitId const wagon_train_id    = wagon_train.id();
    UnitId const pioneer_id        = pioneer.id();
    UnitId const caravel_id        = caravel.id();
    UnitId const frigate_id        = frigate.id();
    UnitId const onboard_id        = onboard.id();

    // Note that, in the OG, the brave attacking a colony is al-
    // ways destroyed after an attack on a colony, provided the
    // colony is not destroyed in the process. If it is destroyed
    // then the brave survives.
    CombatBraveAttackColony const combat{
      .winner           = e_combat_winner::attacker,
      .colony_id        = colony.id,
      .colony_destroyed = true,
      .attacker         = { .id = attacker.id,
                            .outcome =
                                NativeUnitCombatOutcome::no_change{} },
      .defender         = {
                .id      = defender_id,
                .outcome = EuroUnitCombatOutcome::destroyed{} } };
    // Select which colony worker to be the defender. In this
    // case there is only one. This is done because there are not
    // military units.
    W.rand().EXPECT__between_ints( 0, 0 ).returns( 0 );
    W.combat()
        .EXPECT__brave_attack_colony( attacker, defender,
                                      colony )
        .returns( combat );
    mock_land_view.EXPECT__ensure_visible( attacker_coord );
    mock_land_view.EXPECT__ensure_visible( defender_coord );
    mock_euro_agent.EXPECT__message_box(
        "[Arawaks] make surprise raid of [1]! Terror "
        "descends upon colonists! Arawak chief unavailable "
        "for comment." );
    mock_euro_agent.EXPECT__message_box(
        "Colonists on ships docked in [1] have been offboarded "
        "to help defend the colony!" );
    mock_land_view.EXPECT__animate( _ );
    mock_euro_agent.EXPECT__message_box(
        "Port in [1] contained one [Caravel] that was damaged "
        "in battle and was sent to [Amsterdam] for repairs." );
    mock_euro_agent.EXPECT__message_box(
        "Port in [1] contained one [Frigate] that was damaged "
        "in battle and was sent to [Amsterdam] for repairs." );
    mock_euro_agent.EXPECT__show_woodcut(
        e_woodcut::colony_burning );
    mock_euro_agent.EXPECT__message_box(
        "[Arawak] massacre [Dutch] population in [1]! "
        "Colony set ablaze and decimated! The King demands "
        "accountability!" );
    mock_native_agent.EXPECT__on_attack_colony_finished(
        combat, BraveAttackColonyEffect::none{} );
    f( attacker );

    REQUIRE( player.money == 1000 );
    REQUIRE( W.natives().dwelling_exists( dwelling_id ) );
    REQUIRE( W.units().exists( attacker_id ) );
    REQUIRE_FALSE( W.colonies().exists( colony_id ) );
    REQUIRE_FALSE( W.units().exists( defender_id ) );
    REQUIRE_FALSE( W.units().exists( worker_id ) );
    REQUIRE_FALSE( W.units().exists( onboard_id ) );
    REQUIRE_FALSE( W.units().exists( wagon_train_id ) );
    REQUIRE_FALSE( W.units().exists( pioneer_id ) );
    REQUIRE_FALSE( W.square( colony_location ).road );

    REQUIRE( W.units().exists( caravel_id ) );
    REQUIRE( W.units().exists( frigate_id ) );
    REQUIRE( W.units()
                 .unit_for( caravel_id )
                 .orders()
                 .holds<unit_orders::damaged>() );
    REQUIRE( W.units()
                 .unit_for( frigate_id )
                 .orders()
                 .holds<unit_orders::damaged>() );
    REQUIRE( as_const( W.units() ).ownership_of( caravel_id ) ==
             UnitOwnership::harbor{
               .port_status = PortStatus::in_port{} } );
    REQUIRE( as_const( W.units() ).ownership_of( frigate_id ) ==
             UnitOwnership::harbor{
               .port_status = PortStatus::in_port{} } );
  }
}

} // namespace
} // namespace rn
