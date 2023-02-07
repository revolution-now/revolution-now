/****************************************************************
**anim-builders.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-30.
*
* Description: Unit tests for the src/anim-builders.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/anim-builders.hpp"

// Testing
#include "test/fake/world.hpp"

// Revolution Now
#include "icombat.rds.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/native-unit.rds.hpp"
#include "ss/ref.hpp"
#include "ss/unit.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

namespace P = AnimationPrimitive;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, //
        _, L, L, //
        _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[anim-builders] anim_seq_for_unit_move" ) {
  AnimationSequence expected;
  UnitId            unit_id   = {};
  e_direction       direction = {};

  auto f = [&] {
    return anim_seq_for_unit_move( unit_id, direction );
  };

  unit_id   = UnitId{ 3 };
  direction = e_direction::s;
  expected  = {
       .sequence = {
          { { .primitive =
                   P::slide_unit{ .unit_id   = unit_id,
                                  .direction = direction } },
             { .primitive =
                   P::play_sound{ .what = e_sfx::move } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_boarding_ship" ) {
  AnimationSequence expected;
  UnitId            unit_id   = {};
  UnitId            ship_id   = {};
  e_direction       direction = {};

  auto f = [&] {
    return anim_seq_for_boarding_ship( unit_id, ship_id,
                                       direction );
  };

  unit_id   = UnitId{ 3 };
  ship_id   = UnitId{ 4 };
  direction = e_direction::s;
  expected  = {
       .sequence = {
          { { .primitive  = P::front_unit{ .unit_id = ship_id },
               .background = true },
             { .primitive =
                   P::slide_unit{ .unit_id   = unit_id,
                                  .direction = direction } },
             { .primitive =
                   P::play_sound{ .what = e_sfx::move } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_unit_depixelation" ) {
  AnimationSequence expected;
  UnitId            unit_id = {};

  auto f = [&] {
    return anim_seq_for_unit_depixelation( unit_id );
  };

  unit_id  = UnitId{ 3 };
  expected = {
      .sequence = {
          { { .primitive =
                  P::depixelate_unit{ .unit_id = unit_id } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_lost } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE(
    "[anim-builders] anim_seq_for_unit_depixelation with "
    "target" ) {
  AnimationSequence expected;
  UnitId            unit_id     = {};
  e_unit_type       target_type = {};

  auto f = [&] {
    return anim_seq_for_unit_depixelation( unit_id,
                                           target_type );
  };

  unit_id     = UnitId{ 3 };
  target_type = e_unit_type::expert_fur_trapper;
  expected    = {
         .sequence = {
          { { .primitive = P::depixelate_euro_unit_to_target{
                     .unit_id = unit_id,
                     .target  = target_type } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE(
    "[anim-builders] anim_seq_for_unit_depixelation (native)" ) {
  AnimationSequence  expected;
  NativeUnitId       unit_id     = {};
  e_native_unit_type target_type = {};

  auto f = [&] {
    return anim_seq_for_unit_depixelation( unit_id,
                                           target_type );
  };

  unit_id     = NativeUnitId{ 3 };
  target_type = e_native_unit_type::mounted_warrior;
  expected    = {
         .sequence = {
          { { .primitive = P::depixelate_native_unit_to_target{
                     .unit_id = unit_id,
                     .target  = target_type } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_unit_enpixelation" ) {
  AnimationSequence expected;
  UnitId            unit_id = {};

  auto f = [&] {
    return anim_seq_for_unit_enpixelation( unit_id );
  };

  unit_id  = UnitId{ 3 };
  expected = {
      .sequence = { { { .primitive = P::enpixelate_unit{
                            .unit_id = unit_id } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_convert_produced" ) {
  AnimationSequence expected;
  UnitId            unit_id   = {};
  e_direction       direction = {};

  auto f = [&] {
    return anim_seq_for_convert_produced( unit_id, direction );
  };

  unit_id   = UnitId{ 3 };
  direction = e_direction::w;
  expected  = {
       .sequence = {
          /*phase1=*/{
              { .primitive =
                     P::enpixelate_unit{ .unit_id = unit_id } } },
          /*phase2=*/{
              { .primitive =
                     P::slide_unit{ .unit_id   = unit_id,
                                    .direction = direction } },
              { .primitive =
                     P::play_sound{ e_sfx::move } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_colony_depixelation" ) {
  AnimationSequence expected;
  ColonyId          colony_id = {};

  auto f = [&] {
    return anim_seq_for_colony_depixelation( colony_id );
  };

  colony_id = ColonyId{ 3 };
  expected  = {
       .sequence = {
          { { .primitive =
                   P::depixelate_colony{ .colony_id =
                                            colony_id } },
             { .primitive = P::play_sound{
                   .what = e_sfx::city_destroyed } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_unit_to_front" ) {
  AnimationSequence expected;
  UnitId            unit_id = {};

  auto f = [&] { return anim_seq_unit_to_front( unit_id ); };

  unit_id  = UnitId{ 3 };
  expected = {
      .sequence = {
          { { .primitive  = P::front_unit{ .unit_id = unit_id },
              .background = true } } } };
  REQUIRE( f() == expected );
}

TEST_CASE(
    "[anim-builders] anim_seq_unit_to_front_non_background" ) {
  AnimationSequence expected;
  UnitId            unit_id = {};

  auto f = [&] {
    return anim_seq_unit_to_front_non_background( unit_id );
  };

  unit_id  = UnitId{ 3 };
  expected = {
      .sequence = {
          { { .primitive  = P::front_unit{ .unit_id = unit_id },
              .background = false } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_attack_euro" ) {
  World             W;
  AnimationSequence expected;
  Unit const& attacker = W.add_unit_on_map( e_unit_type::soldier,
                                            { .x = 1, .y = 0 } );
  Unit const& defender = W.add_unit_on_map( e_unit_type::soldier,
                                            { .x = 1, .y = 1 } );
  CombatEuroAttackEuro combat{
      .attacker = { .id = attacker.id() },
      .defender = { .id = defender.id() } };

  auto f = [&] {
    return anim_seq_for_attack_euro( W.ss(), combat );
  };

  expected = {
      .sequence = { /*phase 1=*/{
          { .primitive =
                P::front_unit{ .unit_id = defender.id() },
            .background = true },
          { .primitive =
                P::slide_unit{ .unit_id   = attacker.id(),
                               .direction = e_direction::s } },
          { .primitive =
                P::play_sound{ .what = e_sfx::move } } } } };

  SECTION( "attacker wins" ) {
    combat.winner = e_combat_winner::attacker;

    combat.attacker.outcome = EuroUnitCombatOutcome::no_change{};
    combat.defender.outcome = EuroUnitCombatOutcome::destroyed{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::front_unit{ .unit_id = attacker.id() },
              .background = true },
            { .primitive =
                  P::depixelate_unit{ .unit_id =
                                          defender.id() } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_won } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "defender wins" ) {
    combat.winner = e_combat_winner::defender;

    combat.attacker.outcome = EuroUnitCombatOutcome::demoted{
        .to = e_unit_type::free_colonist };
    combat.defender.outcome = EuroUnitCombatOutcome::no_change{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::depixelate_euro_unit_to_target{
                      .unit_id = attacker.id(),
                      .target  = e_unit_type::free_colonist } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id() },
              .background = true },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_lost } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "attacker wins (capture)" ) {
    combat.winner = e_combat_winner::attacker;

    combat.attacker.outcome = EuroUnitCombatOutcome::no_change{};
    combat.defender.outcome = EuroUnitCombatOutcome::captured{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::front_unit{ .unit_id = attacker.id() },
              .background = true },
            { .primitive =
                  P::depixelate_unit{ .unit_id =
                                          defender.id() } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_won } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "attacker wins (capture_and_demote)" ) {
    combat.winner = e_combat_winner::attacker;

    combat.attacker.outcome = EuroUnitCombatOutcome::no_change{};
    combat.defender.outcome =
        EuroUnitCombatOutcome::captured_and_demoted{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::front_unit{ .unit_id = attacker.id() },
              .background = true },
            { .primitive =
                  P::depixelate_unit{ .unit_id =
                                          defender.id() } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_won } } } );
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[anim-builders] anim_seq_for_attack_brave" ) {
  World             W;
  AnimationSequence expected;

  Dwelling& dwelling =
      W.add_dwelling( { .x = 2, .y = 1 }, e_tribe::sioux );
  Unit const& attacker = W.add_unit_on_map( e_unit_type::soldier,
                                            { .x = 1, .y = 0 } );
  NativeUnit const& defender = W.add_native_unit_on_map(
      e_native_unit_type::mounted_brave, { .x = 1, .y = 1 },
      dwelling.id );
  CombatEuroAttackBrave combat{
      .attacker = { .id = attacker.id() },
      .defender = { .id = defender.id } };

  auto f = [&] {
    return anim_seq_for_attack_brave( W.ss(), combat );
  };

  expected = {
      .sequence = { /*phase 1=*/{
          { .primitive = P::front_unit{ .unit_id = defender.id },
            .background = true },
          { .primitive =
                P::slide_unit{ .unit_id   = attacker.id(),
                               .direction = e_direction::s } },
          { .primitive =
                P::play_sound{ .what = e_sfx::move } } } } };

  SECTION( "attacker wins" ) {
    combat.winner = e_combat_winner::attacker;

    combat.attacker.outcome = EuroUnitCombatOutcome::no_change{};
    combat.defender.outcome =
        NativeUnitCombatOutcome::destroyed{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::front_unit{ .unit_id = attacker.id() },
              .background = true },
            { .primitive =
                  P::depixelate_unit{ .unit_id = defender.id } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_won } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "defender wins" ) {
    combat.winner = e_combat_winner::defender;

    combat.attacker.outcome = EuroUnitCombatOutcome::demoted{
        .to = e_unit_type::free_colonist };
    combat.defender.outcome =
        NativeUnitCombatOutcome::no_change{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::depixelate_euro_unit_to_target{
                      .unit_id = attacker.id(),
                      .target  = e_unit_type::free_colonist } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id },
              .background = true },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_lost } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "defender wins (promoted)" ) {
    combat.winner = e_combat_winner::defender;

    combat.attacker.outcome = EuroUnitCombatOutcome::demoted{
        .to = e_unit_type::free_colonist };
    combat.defender.outcome = NativeUnitCombatOutcome::promoted{
        .to = e_native_unit_type::mounted_warrior };

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::depixelate_euro_unit_to_target{
                      .unit_id = attacker.id(),
                      .target  = e_unit_type::free_colonist } },
            { .primitive =
                  P::depixelate_native_unit_to_target{
                      .unit_id = defender.id,
                      .target  = e_native_unit_type::
                          mounted_warrior } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_lost } } } );
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[anim-builders] anim_seq_for_naval_battle" ) {
  World             W;
  AnimationSequence expected;
  Unit const&       attacker = W.add_unit_on_map(
      e_unit_type::privateer, { .x = 0, .y = 0 } );
  Unit const& defender = W.add_unit_on_map( e_unit_type::frigate,
                                            { .x = 0, .y = 1 } );
  CombatShipAttackShip combat{
      .attacker = { .id = attacker.id() },
      .defender = { .id = defender.id() } };

  auto f = [&] {
    return anim_seq_for_naval_battle( W.ss(), combat );
  };

  expected = {
      .sequence = { /*phase 1=*/{
          { .primitive =
                P::front_unit{ .unit_id = defender.id() },
            .background = true },
          { .primitive =
                P::slide_unit{ .unit_id   = attacker.id(),
                               .direction = e_direction::s } },
          { .primitive =
                P::play_sound{ .what = e_sfx::move } } } } };

  SECTION( "evade" ) {
    combat.winner  = e_combat_winner::defender;
    combat.outcome = e_naval_combat_outcome::evade;

    combat.attacker.outcome =
        EuroNavalUnitCombatOutcome::no_change{};
    combat.defender.outcome =
        EuroNavalUnitCombatOutcome::no_change{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::front_unit{ .unit_id = attacker.id() },
              .background = true },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id() },
              .background = true },
            { .primitive =
                  P::play_sound{ .what = e_sfx::move } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "attacker wins" ) {
    combat.winner  = e_combat_winner::attacker;
    combat.outcome = e_naval_combat_outcome::damaged;

    combat.attacker.outcome = EuroNavalUnitCombatOutcome::moved{
        .to{ .x = 0, .y = 1 } };
    combat.defender.outcome =
        EuroNavalUnitCombatOutcome::damaged{};

    expected.sequence.push_back(
        /*phase 2=*/{ { .primitive =
                            P::depixelate_unit{
                                .unit_id = defender.id() } },
                      { .primitive = P::play_sound{
                            .what = e_sfx::attacker_won } } } );
    expected.sequence.push_back(
        /*phase 3=*/{
            { .primitive =
                  P::hide_unit{ .unit_id = defender.id() },
              .background = true },
            { .primitive =
                  P::slide_unit{ .unit_id   = attacker.id(),
                                 .direction = e_direction::s } },
            { .primitive =
                  P::play_sound{ .what = e_sfx::move } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "defender wins" ) {
    combat.winner  = e_combat_winner::defender;
    combat.outcome = e_naval_combat_outcome::sunk;

    combat.attacker.outcome = EuroNavalUnitCombatOutcome::sunk{};
    combat.defender.outcome =
        EuroNavalUnitCombatOutcome::no_change{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::depixelate_unit{ .unit_id =
                                          attacker.id() } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id() },
              .background = true },
            { .primitive = P::play_sound{
                  .what = e_sfx::sunk_ship } } } );
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[anim-builders] anim_seq_for_undefended_colony" ) {
  World             W;
  AnimationSequence expected;
  Unit const& attacker = W.add_unit_on_map( e_unit_type::soldier,
                                            { .x = 1, .y = 0 } );
  Colony const& colony = W.add_colony( { .x = 1, .y = 1 } );
  Unit const&   defender = W.add_unit_outdoors(
      colony.id, e_direction::n, e_outdoor_job::food );
  CombatEuroAttackUndefendedColony combat{
      .colony_id = colony.id,
      .attacker  = { .id = attacker.id() },
      .defender  = { .id = defender.id() } };

  auto f = [&] {
    return anim_seq_for_undefended_colony( W.ss(), combat );
  };

  expected = {
      .sequence = { /*phase 1=*/{
          { .primitive =
                P::front_unit{ .unit_id = defender.id() },
            .background = true },
          { .primitive =
                P::slide_unit{ .unit_id   = attacker.id(),
                               .direction = e_direction::s } },
          { .primitive =
                P::play_sound{ .what = e_sfx::move } } } } };

  SECTION( "attacker wins" ) {
    combat.winner = e_combat_winner::attacker;

    combat.attacker.outcome = EuroUnitCombatOutcome::no_change{};
    combat.defender.outcome =
        EuroColonyWorkerCombatOutcome::defeated{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::front_unit{ .unit_id = attacker.id() },
              .background = true },
            { .primitive =
                  P::depixelate_unit{ .unit_id =
                                          defender.id() } },
            { .primitive = P::play_sound{
                  .what = e_sfx::city_destroyed } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "defender wins" ) {
    combat.winner = e_combat_winner::defender;

    combat.attacker.outcome = EuroUnitCombatOutcome::demoted{
        .to = e_unit_type::free_colonist };
    combat.defender.outcome =
        EuroColonyWorkerCombatOutcome::no_change{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::depixelate_euro_unit_to_target{
                      .unit_id = attacker.id(),
                      .target  = e_unit_type::free_colonist } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id() },
              .background = true },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_lost } } } );
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[anim-builders] anim_seq_for_dwelling_burn" ) {
  World             W;
  AnimationSequence expected;
  Unit const& attacker = W.add_unit_on_map( e_unit_type::soldier,
                                            { .x = 1, .y = 0 } );
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::apache );
  // Create phantom brave that defends the dwelling.
  NativeUnitId const defender_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 1 }, dwelling.id )
          .id;
  NativeUnitId const other_brave_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 0, .y = 1 }, dwelling.id )
          .id;
  EuroUnitCombatOutcome::promoted const attacker_outcome{
      .to = e_unit_type::veteran_soldier };
  DwellingCombatOutcome::destruction const dwelling_destruction{
      .braves_to_kill = { other_brave_id } };

  auto f = [&] {
    return anim_seq_for_dwelling_burn(
        W.ss(), attacker.id(), attacker_outcome, defender_id,
        dwelling.id, dwelling_destruction );
  };

  expected = {
      .sequence = {
          /*phase 1=*/{
              { .primitive =
                    P::front_unit{ .unit_id = defender_id },
                .background = true },
              { .primitive =
                    P::slide_unit{
                        .unit_id   = attacker.id(),
                        .direction = e_direction::s } },
              { .primitive =
                    P::play_sound{ .what = e_sfx::move } } },
          /*phase 2=*/{
              { .primitive =
                    P::depixelate_euro_unit_to_target{
                        .unit_id = attacker.id(),
                        .target =
                            e_unit_type::veteran_soldier } },
              { .primitive =
                    P::depixelate_unit{ .unit_id =
                                            defender_id } },
              { .primitive =
                    P::depixelate_dwelling{ .dwelling_id =
                                                dwelling.id } },
              { .primitive =
                    P::depixelate_unit{ .unit_id =
                                            other_brave_id } },
              { .primitive = P::play_sound{
                    .what = e_sfx::city_destroyed } } } } };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
