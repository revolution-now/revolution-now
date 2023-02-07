/****************************************************************
**combat.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-30.
*
* Description: Unit tests for the src/combat.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/combat.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/irand.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/native-unit.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/tribe.rds.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::Approx;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_nation::english );
    add_player( e_nation::french );
    set_default_player( e_nation::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, //
        L, L, L, //
        _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }

  void expect_promotion( bool promoted ) {
    EXPECT_CALL( rand(), bernoulli( .45 ) ).returns( promoted );
  }

  void expect_treasure_amount( double probability, int min,
                               int max, maybe<int> amount ) {
    if( !amount.has_value() ) {
      EXPECT_CALL( rand(), bernoulli( probability ) )
          .returns( false );
      return;
    } else {
      EXPECT_CALL( rand(), bernoulli( probability ) )
          .returns( true );
      EXPECT_CALL( rand(),
                   between_ints( min, max, e_interval::closed ) )
          .returns( *amount );
      return;
    }
  }

  void expect_guaranteed_treasure_amount( int min, int max,
                                          int amount ) {
    EXPECT_CALL( rand(),
                 between_ints( min, max, e_interval::closed ) )
        .returns( amount );
  }

  void expect_convert( bool converted, double probability ) {
    EXPECT_CALL( rand(), bernoulli( probability ) )
        .returns( converted );
  }

  void expect_attacker_wins( double probability ) {
    EXPECT_CALL( rand(),
                 bernoulli( Approx( probability, .000001 ) ) )
        .returns( true );
  }

  void expect_defender_wins( double probability ) {
    EXPECT_CALL( rand(),
                 bernoulli( Approx( probability, .000001 ) ) )
        .returns( false );
  }

  void expect_burn_mission( bool burn ) {
    EXPECT_CALL( rand(), bernoulli( .5 ) ).returns( burn );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[combat] combat_euro_attack_euro" ) {
  World                W;
  Player&              english = W.english();
  CombatEuroAttackEuro expected;
  Unit*                attacker = nullptr;
  Unit*                defender = nullptr;
  RealCombat           combat( W.ss(), W.rand() );

  auto f = [&] {
    return combat.euro_attack_euro( *attacker, *defender );
  };

  SECTION( "soldier->soldier, attacker wins, no promotion" ) {
    attacker = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    defender = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 1 },
                                   e_nation::french );
    W.expect_attacker_wins( .5 );
    W.expect_promotion( false );
    expected = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .id     = attacker->id(),
                      .weight = 2.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = { .id      = defender->id(),
                      .weight  = 2.0,
                      .outcome = EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } } };
    REQUIRE( f() == expected );
  }

  SECTION( "soldier->soldier, attacker wins, rand promotion" ) {
    attacker = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    defender = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 1 },
                                   e_nation::french );
    W.expect_attacker_wins( .5 );
    W.expect_promotion( true );
    expected = {
        .winner = e_combat_winner::attacker,
        .attacker =
            { .id     = attacker->id(),
              .weight = 2.0,
              .outcome =
                  EuroUnitCombatOutcome::promoted{
                      .to = e_unit_type::veteran_soldier } },
        .defender = { .id      = defender->id(),
                      .weight  = 2.0,
                      .outcome = EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "soldier(servant)->soldier, attacker wins, promotion via "
      "washington" ) {
    attacker = &W.add_unit_on_map(
        UnitType::create( e_unit_type::soldier,
                          e_unit_type::indentured_servant )
            .value(),
        { .x = 1, .y = 0 }, e_nation::english );
    defender = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 1 },
                                   e_nation::french );
    W.expect_attacker_wins( .5 );
    english.fathers.has[e_founding_father::george_washington] =
        true;
    expected = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .id     = attacker->id(),
                      .weight = 2.0,
                      .outcome =
                          EuroUnitCombatOutcome::promoted{
                              .to = e_unit_type::soldier } },
        .defender = { .id      = defender->id(),
                      .weight  = 2.0,
                      .outcome = EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "soldier(criminal)->soldier, attacker wins, promotion "
      "(criminal)" ) {
    attacker = &W.add_unit_on_map(
        UnitType::create( e_unit_type::soldier,
                          e_unit_type::petty_criminal )
            .value(),
        { .x = 1, .y = 0 }, e_nation::english );
    defender = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 1 },
                                   e_nation::french );
    W.expect_attacker_wins( .5 );
    W.expect_promotion( true );
    expected = {
        .winner = e_combat_winner::attacker,
        .attacker =
            { .id     = attacker->id(),
              .weight = 2.0,
              .outcome =
                  EuroUnitCombatOutcome::promoted{
                      .to = UnitType::create(
                                e_unit_type::soldier,
                                e_unit_type::indentured_servant )
                                .value() } },
        .defender = { .id      = defender->id(),
                      .weight  = 2.0,
                      .outcome = EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "soldier(farmer)->soldier, attacker wins, can't "
      "promote" ) {
    attacker = &W.add_unit_on_map(
        UnitType::create( e_unit_type::soldier,
                          e_unit_type::expert_farmer )
            .value(),
        { .x = 1, .y = 0 }, e_nation::english );
    defender = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 1 },
                                   e_nation::french );
    W.expect_attacker_wins( .5 );
    expected = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .id     = attacker->id(),
                      .weight = 2.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = { .id      = defender->id(),
                      .weight  = 2.0,
                      .outcome = EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "veteran_dragoon->soldier, attacker wins, no "
      "independence" ) {
    attacker = &W.add_unit_on_map( e_unit_type::veteran_dragoon,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    defender = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 1 },
                                   e_nation::french );
    W.expect_attacker_wins( .666666 );
    expected = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .id     = attacker->id(),
                      .weight = 4.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = { .id      = defender->id(),
                      .weight  = 2.0,
                      .outcome = EuroUnitCombatOutcome::demoted{
                          .to = e_unit_type::free_colonist } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "veteran_dragoon->free_colonist, attacker wins, with "
      "independence" ) {
    attacker = &W.add_unit_on_map( e_unit_type::veteran_dragoon,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    defender = &W.add_unit_on_map( e_unit_type::free_colonist,
                                   { .x = 1, .y = 1 },
                                   e_nation::french );
    W.expect_attacker_wins( .8 );
    W.expect_promotion( true );
    english.revolution_status = e_revolution_status::declared;

    expected = {
        .winner = e_combat_winner::attacker,
        .attacker =
            { .id     = attacker->id(),
              .weight = 4.0,
              .outcome =
                  EuroUnitCombatOutcome::promoted{
                      .to = e_unit_type::continental_cavalry } },
        .defender = { .id      = defender->id(),
                      .weight  = 1.0,
                      .outcome = EuroUnitCombatOutcome::captured{
                          .new_nation = e_nation::english,
                          .new_coord  = { .x = 1, .y = 0 } } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "soldier->veteran_colonist, attacker wins, "
      "capture_and_demote" ) {
    attacker = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    defender = &W.add_unit_on_map( e_unit_type::veteran_colonist,
                                   { .x = 1, .y = 1 },
                                   e_nation::french );
    W.expect_attacker_wins( .666666 );
    W.expect_promotion( false );
    expected = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .id     = attacker->id(),
                      .weight = 2.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = {
            .id     = defender->id(),
            .weight = 1.0,
            .outcome =
                EuroUnitCombatOutcome::captured_and_demoted{
                    .to         = e_unit_type::free_colonist,
                    .new_nation = e_nation::english,
                    .new_coord  = { .x = 1, .y = 0 } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "veteran_dragoon->free_colonist, attacker loses" ) {
    attacker = &W.add_unit_on_map( e_unit_type::veteran_dragoon,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    defender = &W.add_unit_on_map( e_unit_type::free_colonist,
                                   { .x = 1, .y = 1 },
                                   e_nation::french );
    W.expect_defender_wins( .8 );
    expected = {
        .winner = e_combat_winner::defender,
        .attacker =
            { .id     = attacker->id(),
              .weight = 4.0,
              .outcome =
                  EuroUnitCombatOutcome::demoted{
                      .to = e_unit_type::veteran_soldier } },
        .defender = {
            .id      = defender->id(),
            .weight  = 1.0,
            .outcome = EuroUnitCombatOutcome::no_change{} } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "artillery->dragoon, attacker loses, defender promoted" ) {
    attacker = &W.add_unit_on_map( e_unit_type::artillery,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    defender = &W.add_unit_on_map( e_unit_type::dragoon,
                                   { .x = 1, .y = 1 },
                                   e_nation::french );
    W.expect_defender_wins( .625 );
    W.expect_promotion( true );
    expected = {
        .winner = e_combat_winner::defender,
        .attacker =
            { .id     = attacker->id(),
              .weight = 5.0,
              .outcome =
                  EuroUnitCombatOutcome::demoted{
                      .to = e_unit_type::damaged_artillery } },
        .defender = {
            .id      = defender->id(),
            .weight  = 3.0,
            .outcome = EuroUnitCombatOutcome::promoted{
                .to = e_unit_type::veteran_dragoon } } };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[combat] combat_euro_attack_undefended_colony" ) {
  World                            W;
  CombatEuroAttackUndefendedColony expected;
  Colony const& colony = W.add_colony( { .x = 1, .y = 1 } );
  Unit const&   defender =
      W.add_unit_indoors( colony.id, e_indoor_job::cigars );
  Unit*      attacker = nullptr;
  RealCombat combat( W.ss(), W.rand() );

  auto f = [&] {
    return combat.euro_attack_undefended_colony(
        *attacker, defender, colony );
  };

  SECTION( "soldier, attacker wins" ) {
    attacker = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    W.expect_attacker_wins( .666666 );
    W.expect_promotion( false );
    expected = {
        .winner    = e_combat_winner::attacker,
        .colony_id = colony.id,
        .attacker  = { .id     = attacker->id(),
                       .weight = 2.0,
                       .outcome =
                           EuroUnitCombatOutcome::no_change{} },
        .defender  = {
             .id     = defender.id(),
             .weight = 1.0,
             .outcome =
                EuroColonyWorkerCombatOutcome::defeated{} } };
    REQUIRE( f() == expected );
  }

  SECTION( "soldier, attacker loses" ) {
    attacker = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    W.expect_defender_wins( .666666 );
    expected = {
        .winner    = e_combat_winner::defender,
        .colony_id = colony.id,
        .attacker  = { .id     = attacker->id(),
                       .weight = 2.0,
                       .outcome =
                           EuroUnitCombatOutcome::demoted{
                               .to =
                                  e_unit_type::free_colonist } },
        .defender  = {
             .id     = defender.id(),
             .weight = 1.0,
             .outcome =
                EuroColonyWorkerCombatOutcome::no_change{} } };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[combat] combat_euro_attack_brave" ) {
  World                 W;
  CombatEuroAttackBrave expected;
  Dwelling const&       dwelling =
      W.add_dwelling( { .x = 2, .y = 1 }, e_tribe::arawak );
  Unit*       attacker = nullptr;
  NativeUnit* defender = nullptr;
  RealCombat  combat( W.ss(), W.rand() );

  auto f = [&] {
    return combat.euro_attack_brave( *attacker, *defender );
  };

  SECTION( "soldier->brave, attacker wins, brave destroyed" ) {
    attacker = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    defender = &W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 1, .y = 1 },
        dwelling.id );
    W.expect_attacker_wins( .666666 );
    W.expect_promotion( false );
    expected = {
        .winner   = e_combat_winner::attacker,
        .attacker = { .id     = attacker->id(),
                      .weight = 2.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },
        .defender = {
            .id      = defender->id,
            .weight  = 1.0,
            .outcome = NativeUnitCombatOutcome::destroyed{
                .tribe_retains_horses  = false,
                .tribe_retains_muskets = false } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "soldier->brave, attacker wins+promoted, brave "
      "destroyed" ) {
    attacker = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    defender = &W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 1, .y = 1 },
        dwelling.id );
    W.expect_attacker_wins( .666666 );
    W.expect_promotion( true );
    expected = {
        .winner = e_combat_winner::attacker,
        .attacker =
            { .id     = attacker->id(),
              .weight = 2.0,
              .outcome =
                  EuroUnitCombatOutcome::promoted{
                      .to = e_unit_type::veteran_soldier } },
        .defender = {
            .id      = defender->id,
            .weight  = 1.0,
            .outcome = NativeUnitCombatOutcome::destroyed{
                .tribe_retains_horses  = false,
                .tribe_retains_muskets = false } } };
    REQUIRE( f() == expected );
  }

  SECTION( "soldier->brave, attacker loses" ) {
    attacker = &W.add_unit_on_map( e_unit_type::soldier,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    defender = &W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 1, .y = 1 },
        dwelling.id );
    W.expect_defender_wins( .666666 );
    expected = {
        .winner   = e_combat_winner::defender,
        .attacker = { .id     = attacker->id(),
                      .weight = 2.0,
                      .outcome =
                          EuroUnitCombatOutcome::demoted{
                              .to =
                                  e_unit_type::free_colonist } },
        .defender = {
            .id      = defender->id,
            .weight  = 1.0,
            .outcome = NativeUnitCombatOutcome::no_change{} } };
    REQUIRE( f() == expected );
  }

  SECTION( "scout->brave, attacker loses" ) {
    attacker = &W.add_unit_on_map( e_unit_type::scout,
                                   { .x = 1, .y = 0 },
                                   e_nation::english );
    defender = &W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 1, .y = 1 },
        dwelling.id );
    W.expect_defender_wins( .5 );
    expected = {
        .winner   = e_combat_winner::defender,
        .attacker = { .id     = attacker->id(),
                      .weight = 1.0,
                      .outcome =
                          EuroUnitCombatOutcome::destroyed{} },
        .defender = {
            .id      = defender->id,
            .weight  = 1.0,
            .outcome = NativeUnitCombatOutcome::no_change{} } };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[combat] combat_euro_attack_dwelling no-burn" ) {
  World                    W;
  CombatEuroAttackDwelling expected;
  e_tribe const            tribe_type = e_tribe::arawak;
  Tribe&                   tribe = W.add_tribe( tribe_type );
  TribeRelationship&       relationship =
      tribe.relationship[W.default_nation()];
  relationship.encountered   = true;
  Coord const kAttackerCoord = { .x = 1, .y = 0 };
  Dwelling&   dwelling =
      W.add_dwelling( { .x = 2, .y = 1 }, tribe_type );
  Unit*      attacker = nullptr;
  RealCombat combat( W.ss(), W.rand() );

  auto f = [&] {
    return combat.euro_attack_dwelling( *attacker, dwelling );
  };

  // Sanity check.
  REQUIRE( relationship.tribal_alarm == 0 );

  SECTION( "soldier, attacker wins" ) {
    attacker =
        &W.add_unit_on_map( e_unit_type::soldier, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .666666 );
    W.expect_promotion( false );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 10,
        .missions_burned  = false,

        .attacker = { .id     = attacker->id(),
                      .weight = 2.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },

        .defender = {
            .id     = dwelling.id,
            .weight = 1.0,
            .outcome =
                DwellingCombatOutcome::population_decrease{
                    .convert_produced = false } } };
    REQUIRE( f() == expected );
  }

  SECTION( "soldier, attacker loses" ) {
    relationship.tribal_alarm = 85;
    attacker =
        &W.add_unit_on_map( e_unit_type::soldier, kAttackerCoord,
                            e_nation::english );
    W.expect_defender_wins( .666666 );
    expected = {
        .winner           = e_combat_winner::defender,
        .new_tribal_alarm = 95,
        .missions_burned  = false,
        .attacker         = { .id     = attacker->id(),
                              .weight = 2.0,
                              .outcome =
                                  EuroUnitCombatOutcome::demoted{
                                      .to =
                                  e_unit_type::free_colonist } },
        .defender         = {
                    .id      = dwelling.id,
                    .weight  = 1.0,
                    .outcome = DwellingCombatOutcome::no_change{} } };
    REQUIRE( f() == expected );
  }

  SECTION( "soldier, attacker loses with population 1" ) {
    dwelling.population = 1;
    attacker =
        &W.add_unit_on_map( e_unit_type::soldier, kAttackerCoord,
                            e_nation::english );
    W.expect_defender_wins( .666666 );
    expected = {
        .winner           = e_combat_winner::defender,
        .new_tribal_alarm = 10,
        .missions_burned  = false,
        .attacker         = { .id     = attacker->id(),
                              .weight = 2.0,
                              .outcome =
                                  EuroUnitCombatOutcome::demoted{
                                      .to =
                                  e_unit_type::free_colonist } },
        .defender         = {
                    .id      = dwelling.id,
                    .weight  = 1.0,
                    .outcome = DwellingCombatOutcome::no_change{} } };
    REQUIRE( f() == expected );
  }

  SECTION( "soldier, attacker loses, with missionary" ) {
    W.add_missionary_in_dwelling( e_unit_type::jesuit_missionary,
                                  dwelling.id )
        .id();
    attacker =
        &W.add_unit_on_map( e_unit_type::soldier, kAttackerCoord,
                            e_nation::english );
    W.expect_defender_wins( .666666 );
    expected = {
        .winner           = e_combat_winner::defender,
        .new_tribal_alarm = 10,
        .missions_burned  = false,
        .attacker         = { .id     = attacker->id(),
                              .weight = 2.0,
                              .outcome =
                                  EuroUnitCombatOutcome::demoted{
                                      .to =
                                  e_unit_type::free_colonist } },
        .defender         = {
                    .id      = dwelling.id,
                    .weight  = 1.0,
                    .outcome = DwellingCombatOutcome::no_change{} } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "soldier, attacker wins+promotion, with missionary, no "
      "convert" ) {
    W.add_missionary_in_dwelling( e_unit_type::jesuit_missionary,
                                  dwelling.id )
        .id();
    attacker =
        &W.add_unit_on_map( e_unit_type::soldier, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .666666 );
    W.expect_promotion( true );
    W.expect_convert( false, .66 );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 10,
        .missions_burned  = false,

        .attacker =
            { .id     = attacker->id(),
              .weight = 2.0,
              .outcome =
                  EuroUnitCombatOutcome::promoted{
                      .to = e_unit_type::veteran_soldier } },

        .defender = {
            .id     = dwelling.id,
            .weight = 1.0,
            .outcome =
                DwellingCombatOutcome::population_decrease{
                    .convert_produced = false } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "soldier, attacker wins, with missionary, with convert" ) {
    W.add_missionary_in_dwelling( e_unit_type::jesuit_missionary,
                                  dwelling.id )
        .id();
    attacker =
        &W.add_unit_on_map( e_unit_type::soldier, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .666666 );
    W.expect_promotion( false );
    W.expect_convert( true, .66 );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 10,
        .missions_burned  = false,

        .attacker = { .id     = attacker->id(),
                      .weight = 2.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },

        .defender = {
            .id     = dwelling.id,
            .weight = 1.0,
            .outcome =
                DwellingCombatOutcome::population_decrease{
                    .convert_produced = true } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "soldier, attacker loses, with missionary, no burn "
      "missions" ) {
    W.add_missionary_in_dwelling( e_unit_type::jesuit_missionary,
                                  dwelling.id )
        .id();
    relationship.tribal_alarm = 85;
    attacker =
        &W.add_unit_on_map( e_unit_type::soldier, kAttackerCoord,
                            e_nation::english );
    W.expect_defender_wins( .666666 );
    W.expect_burn_mission( false );
    expected = {
        .winner           = e_combat_winner::defender,
        .new_tribal_alarm = 95,
        .missions_burned  = false,
        .attacker         = { .id     = attacker->id(),
                              .weight = 2.0,
                              .outcome =
                                  EuroUnitCombatOutcome::demoted{
                                      .to =
                                  e_unit_type::free_colonist } },
        .defender         = {
                    .id      = dwelling.id,
                    .weight  = 1.0,
                    .outcome = DwellingCombatOutcome::no_change{} } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "soldier, attacker loses, with missionary, burn "
      "missions" ) {
    W.add_missionary_in_dwelling( e_unit_type::jesuit_missionary,
                                  dwelling.id )
        .id();
    relationship.tribal_alarm = 85;
    attacker =
        &W.add_unit_on_map( e_unit_type::soldier, kAttackerCoord,
                            e_nation::english );
    W.expect_defender_wins( .666666 );
    W.expect_burn_mission( true );
    expected = {
        .winner           = e_combat_winner::defender,
        .new_tribal_alarm = 95,
        .missions_burned  = true,
        .attacker         = { .id     = attacker->id(),
                              .weight = 2.0,
                              .outcome =
                                  EuroUnitCombatOutcome::demoted{
                                      .to =
                                  e_unit_type::free_colonist } },
        .defender         = {
                    .id      = dwelling.id,
                    .weight  = 1.0,
                    .outcome = DwellingCombatOutcome::no_change{} } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "soldier, attacker wins, with missionary, burn missions, "
      "no convert" ) {
    W.add_missionary_in_dwelling( e_unit_type::jesuit_missionary,
                                  dwelling.id )
        .id();
    relationship.tribal_alarm = 85;
    attacker =
        &W.add_unit_on_map( e_unit_type::soldier, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .666666 );
    W.expect_promotion( false );
    W.expect_burn_mission( true );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 95,
        .missions_burned  = true,

        .attacker = { .id     = attacker->id(),
                      .weight = 2.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },

        .defender = {
            .id     = dwelling.id,
            .weight = 1.0,
            .outcome =
                DwellingCombatOutcome::population_decrease{
                    .convert_produced = false } } };
    REQUIRE( f() == expected );
  }
}

TEST_CASE(
    "[combat] combat_euro_attack_dwelling village-burn" ) {
  World                    W;
  CombatEuroAttackDwelling expected;
  e_tribe const            tribe_type = e_tribe::arawak;
  Tribe&                   tribe = W.add_tribe( tribe_type );
  TribeRelationship&       relationship =
      tribe.relationship[W.default_nation()];
  relationship.encountered   = true;
  Coord const kAttackerCoord = { .x = 1, .y = 0 };
  Dwelling&   dwelling =
      W.add_dwelling( { .x = 2, .y = 1 }, tribe_type );
  NativeUnitId const brave1_id =
      W.add_native_unit_on_map(
           e_native_unit_type::mounted_brave, { .x = 0, .y = 1 },
           dwelling.id )
          .id;
  NativeUnitId const brave2_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 0, .y = 2 }, dwelling.id )
          .id;
  dwelling.population = 1;
  Unit*      attacker = nullptr;
  RealCombat combat( W.ss(), W.rand() );

  auto f = [&] {
    return combat.euro_attack_dwelling( *attacker, dwelling );
  };

  // Sanity check.
  REQUIRE( relationship.tribal_alarm == 0 );

  SECTION( "dragoon, attacker wins, no treasure" ) {
    attacker =
        &W.add_unit_on_map( e_unit_type::dragoon, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .75 );
    W.expect_promotion( false );
    W.expect_treasure_amount( .33, 300, 800, nothing );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 10,
        .missions_burned  = false,

        .attacker = { .id     = attacker->id(),
                      .weight = 3.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },

        .defender = {
            .id      = dwelling.id,
            .weight  = 1.0,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill = { brave1_id, brave2_id },
                .missionary_to_release = nothing,
                .treasure_amount       = nothing,
                .tribe_destroyed       = e_tribe::arawak,
                .convert_produced      = false } } };
    REQUIRE( f() == expected );
  }

  SECTION( "dragoon, attacker wins, max alarm" ) {
    relationship.tribal_alarm = 90;
    attacker =
        &W.add_unit_on_map( e_unit_type::dragoon, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .75 );
    W.expect_promotion( false );
    W.expect_treasure_amount( .33, 300, 800, nothing );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 99,
        .missions_burned  = false,

        .attacker = { .id     = attacker->id(),
                      .weight = 3.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },

        .defender = {
            .id      = dwelling.id,
            .weight  = 1.0,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill = { brave1_id, brave2_id },
                .missionary_to_release = nothing,
                .treasure_amount       = nothing,
                .tribe_destroyed       = e_tribe::arawak,
                .convert_produced      = false } } };
    REQUIRE( f() == expected );
  }

  SECTION( "dragoon, attacker wins, capital" ) {
    dwelling.is_capital       = true;
    relationship.tribal_alarm = 90;
    attacker =
        &W.add_unit_on_map( e_unit_type::dragoon, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .75 );
    W.expect_promotion( false );
    // This will produce a treasure amount of 345, which will
    // then be rounded down to 300, and doubled because this is a
    // capital.
    W.expect_guaranteed_treasure_amount( 300, 800, 345 );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 17,
        .missions_burned  = false,

        .attacker = { .id     = attacker->id(),
                      .weight = 3.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },

        .defender = {
            .id      = dwelling.id,
            .weight  = 1.0,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill = { brave1_id, brave2_id },
                .missionary_to_release = nothing,
                .treasure_amount       = 600,
                .tribe_destroyed       = e_tribe::arawak,
                .convert_produced      = false } } };
    REQUIRE( f() == expected );
  }

  SECTION( "dragoon, attacker wins, cortes" ) {
    W.default_player()
        .fathers.has[e_founding_father::hernan_cortes] = true;
    attacker =
        &W.add_unit_on_map( e_unit_type::dragoon, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .75 );
    W.expect_promotion( false );
    // This will produce a treasure amount of 345, which will be
    // multiplied by 1.5 to get 517 because the player has
    // cortes, then rounded down to 500.
    W.expect_guaranteed_treasure_amount( 300, 800, 345 );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 10,
        .missions_burned  = false,

        .attacker = { .id     = attacker->id(),
                      .weight = 3.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },

        .defender = {
            .id      = dwelling.id,
            .weight  = 1.0,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill = { brave1_id, brave2_id },
                .missionary_to_release = nothing,
                .treasure_amount       = 500,
                .tribe_destroyed       = e_tribe::arawak,
                .convert_produced      = false } } };
    REQUIRE( f() == expected );
  }

  SECTION( "dragoon, attacker wins, with treasure" ) {
    attacker =
        &W.add_unit_on_map( e_unit_type::dragoon, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .75 );
    W.expect_promotion( false );
    // This will produce a treasure amount of 345, which will
    // then be rounded down to 300.
    W.expect_treasure_amount( .33, 300, 800, 345 );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 10,
        .missions_burned  = false,

        .attacker = { .id     = attacker->id(),
                      .weight = 3.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },

        .defender = {
            .id      = dwelling.id,
            .weight  = 1.0,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill = { brave1_id, brave2_id },
                .missionary_to_release = nothing,
                .treasure_amount       = 300,
                .tribe_destroyed       = e_tribe::arawak,
                .convert_produced      = false } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "dragoon, attacker wins, tribe not destroyed, no "
      "braves" ) {
    W.add_dwelling( { .x = 2, .y = 2 }, tribe_type );
    W.units().destroy_unit( brave1_id );
    W.units().destroy_unit( brave2_id );
    attacker =
        &W.add_unit_on_map( e_unit_type::dragoon, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .75 );
    W.expect_promotion( false );
    W.expect_treasure_amount( .33, 300, 800, nothing );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 10,
        .missions_burned  = false,

        .attacker = { .id     = attacker->id(),
                      .weight = 3.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },

        .defender = {
            .id      = dwelling.id,
            .weight  = 1.0,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill        = {},
                .missionary_to_release = nothing,
                .treasure_amount       = nothing,
                .tribe_destroyed       = nothing,
                .convert_produced      = false } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "dragoon, attacker wins, missionary, no missions "
      "burned, with convert" ) {
    UnitId const missionary_id =
        W.add_missionary_in_dwelling(
             UnitType::create( e_unit_type::missionary,
                               e_unit_type::indentured_servant )
                 .value(),
             dwelling.id )
            .id();
    attacker =
        &W.add_unit_on_map( e_unit_type::dragoon, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .75 );
    W.expect_promotion( false );
    W.expect_convert( true, .22 );
    W.expect_treasure_amount( .33, 300, 800, nothing );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 10,
        .missions_burned  = false,

        .attacker = { .id     = attacker->id(),
                      .weight = 3.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },

        .defender = {
            .id      = dwelling.id,
            .weight  = 1.0,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill = { brave1_id, brave2_id },
                .missionary_to_release = missionary_id,
                .treasure_amount       = nothing,
                .tribe_destroyed       = e_tribe::arawak,
                .convert_produced      = true } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "dragoon, attacker wins, foreign missionary not released, "
      "no missions burned, with no convert" ) {
    W.add_missionary_in_dwelling(
         UnitType::create( e_unit_type::missionary,
                           e_unit_type::indentured_servant )
             .value(),
         dwelling.id, e_nation::french )
        .id();
    attacker =
        &W.add_unit_on_map( e_unit_type::dragoon, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .75 );
    W.expect_promotion( false );
    W.expect_treasure_amount( .33, 300, 800, nothing );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 10,
        .missions_burned  = false,

        .attacker = { .id     = attacker->id(),
                      .weight = 3.0,
                      .outcome =
                          EuroUnitCombatOutcome::no_change{} },

        .defender = {
            .id      = dwelling.id,
            .weight  = 1.0,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill = { brave1_id, brave2_id },
                .missionary_to_release = nothing,
                .treasure_amount       = nothing,
                .tribe_destroyed       = e_tribe::arawak,
                .convert_produced      = false } } };
    REQUIRE( f() == expected );
  }

  SECTION(
      "dragoon, attacker wins, missionary, missions burned" ) {
    relationship.tribal_alarm = 90;
    W.add_missionary_in_dwelling(
         UnitType::create( e_unit_type::missionary,
                           e_unit_type::indentured_servant )
             .value(),
         dwelling.id )
        .id();
    attacker =
        &W.add_unit_on_map( e_unit_type::dragoon, kAttackerCoord,
                            e_nation::english );
    W.expect_attacker_wins( .75 );
    W.expect_promotion( true );
    W.expect_burn_mission( true );
    W.expect_treasure_amount( .33, 300, 800, nothing );
    expected = {
        .winner           = e_combat_winner::attacker,
        .new_tribal_alarm = 99,
        .missions_burned  = true,

        .attacker =
            { .id     = attacker->id(),
              .weight = 3.0,
              .outcome =
                  EuroUnitCombatOutcome::promoted{
                      .to = e_unit_type::veteran_dragoon } },

        .defender = {
            .id      = dwelling.id,
            .weight  = 1.0,
            .outcome = DwellingCombatOutcome::destruction{
                .braves_to_kill = { brave1_id, brave2_id },
                .missionary_to_release = nothing,
                .treasure_amount       = nothing,
                .tribe_destroyed       = e_tribe::arawak,
                .convert_produced      = false } } };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[combat] ship_attack_ship" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
