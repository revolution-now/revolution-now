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
#include "imap-updater.hpp"
#include "visibility.hpp"

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

using P = AnimationPrimitive;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_nation::french );
    set_default_player( e_nation::french );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, L, L, //
        _, L, L, L, L, //
        _, L, L, L, L, //
        _, L, L, L, L, //
        _, L, L, L, L, //
    };
    build_map( std::move( tiles ), 5 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[anim-builders] anim_seq_for_unit_move" ) {
  World             W;
  AnimationSequence expected;
  UnitId            unit_id   = {};
  e_direction       direction = {};
  Coord             coord     = {};

  auto f = [&] {
    return anim_seq_for_unit_move( W.ss(), unit_id, direction );
  };

  coord = { .x = 1, .y = 1 };
  unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord )
          .id();
  direction = e_direction::s;
  expected  = {
       .sequence = {
          /*phase 0*/ {
              { .primitive =
                     P::ensure_tile_visible{ .tile = coord } },
              { .primitive =
                     P::ensure_tile_visible{
                         .tile = coord.moved( direction ) } } },
          /*phase 1*/ {
              { .primitive =
                     P::slide_unit{ .unit_id   = unit_id,
                                    .direction = direction } },
              { .primitive =
                     P::play_sound{ .what = e_sfx::move } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_unit_move off map" ) {
  World             W;
  AnimationSequence expected;
  UnitId            unit_id   = {};
  e_direction       direction = {};
  Coord             coord     = {};

  auto f = [&] {
    return anim_seq_for_unit_move( W.ss(), unit_id, direction );
  };

  coord = { .x = 1, .y = 1 };
  unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord )
          .id();
  direction = e_direction::n;
  expected  = {
       .sequence = {
          /*phase 0*/ {
              { .primitive =
                     P::ensure_tile_visible{ .tile = coord } },
              { .primitive =
                     P::ensure_tile_visible{
                         .tile = coord.moved( direction ) } } },
          /*phase 1*/ {
              { .primitive =
                     P::slide_unit{ .unit_id   = unit_id,
                                    .direction = direction } },
              { .primitive =
                     P::play_sound{ .what = e_sfx::move } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_boarding_ship" ) {
  World             W;
  AnimationSequence expected;
  UnitId            unit_id    = {};
  UnitId            ship_id    = {};
  e_direction       direction  = {};
  Coord             unit_coord = {};
  Coord             ship_coord = {};

  auto f = [&] {
    return anim_seq_for_boarding_ship( W.ss(), unit_id, ship_id,
                                       direction );
  };

  unit_coord = { .x = 1, .y = 1 };
  unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, unit_coord )
          .id();
  ship_coord = { .x = 0, .y = 1 };
  ship_id =
      W.add_unit_on_map( e_unit_type::free_colonist, ship_coord )
          .id();
  direction = e_direction::w;
  expected  = {
       .sequence = {
          /*phase 0*/ { { .primitive =
                               P::ensure_tile_visible{
                                   .tile = unit_coord } },
                         { .primitive =
                               P::ensure_tile_visible{
                                   .tile = ship_coord } } },
          /*phase 1*/
          { { .primitive = P::front_unit{ .unit_id = ship_id } },
             { .primitive =
                   P::slide_unit{ .unit_id   = unit_id,
                                  .direction = direction } },
             { .primitive =
                   P::play_sound{ .what = e_sfx::move } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_unit_depixelation" ) {
  World             W;
  AnimationSequence expected;
  UnitId            unit_id = {};
  Coord             coord   = {};

  auto f = [&] {
    return anim_seq_for_unit_depixelation( W.ss(), unit_id );
  };

  coord = { .x = 1, .y = 1 };
  unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord )
          .id();
  expected = {
      .sequence = {
          /*phase 0*/ {
              { .primitive =
                    P::ensure_tile_visible{ .tile = coord } } },
          /*phase 1*/ {
              { .primitive =
                    P::depixelate_euro_unit{ .unit_id =
                                                 unit_id } },
              { .primitive = P::play_sound{
                    .what = e_sfx::attacker_lost } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE(
    "[anim-builders] anim_seq_for_unit_depixelation with "
    "target" ) {
  World             W;
  AnimationSequence expected;
  UnitId            unit_id     = {};
  e_unit_type       target_type = {};
  Coord             coord       = {};

  auto f = [&] {
    return anim_seq_for_unit_depixelation( W.ss(), unit_id,
                                           target_type );
  };

  coord = { .x = 1, .y = 1 };
  unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord )
          .id();
  target_type = e_unit_type::expert_fur_trapper;
  expected    = {
         .sequence = {
          /*phase 0*/ {
              { .primitive =
                       P::ensure_tile_visible{ .tile = coord } } },
          /*phase 1*/ {
              { .primitive = P::pixelate_euro_unit_to_target{
                       .unit_id = unit_id,
                       .target  = target_type } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE(
    "[anim-builders] anim_seq_for_unit_depixelation (native)" ) {
  World              W;
  AnimationSequence  expected;
  NativeUnitId       unit_id     = {};
  e_native_unit_type target_type = {};
  Coord              coord       = {};

  auto f = [&] {
    return anim_seq_for_unit_depixelation( W.ss(), unit_id,
                                           target_type );
  };

  coord = { .x = 1, .y = 1 };
  DwellingId const dwelling_id =
      W.add_dwelling( coord, e_tribe::inca ).id;
  unit_id = W.add_native_unit_on_map( e_native_unit_type::brave,
                                      coord, dwelling_id )
                .id;
  target_type = e_native_unit_type::mounted_warrior;
  expected    = {
         .sequence = {
          /*phase 0*/ {
              { .primitive =
                       P::ensure_tile_visible{ .tile = coord } } },
          /*phase 1*/ {
              { .primitive = P::pixelate_native_unit_to_target{
                       .unit_id = unit_id,
                       .target  = target_type } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_unit_enpixelation" ) {
  World             W;
  AnimationSequence expected;
  UnitId            unit_id = {};
  Coord             coord   = {};

  auto f = [&] {
    return anim_seq_for_unit_enpixelation( W.ss(), unit_id );
  };

  coord = { .x = 1, .y = 1 };
  unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord )
          .id();
  expected = {
      .sequence = {
          /*phase 0*/ {
              { .primitive =
                    P::ensure_tile_visible{ .tile = coord } } },
          /*phase 1*/ { { .primitive = P::enpixelate_unit{
                              .unit_id = unit_id } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE(
    "[anim-builders] anim_seq_for_treasure_enpixelation" ) {
  World             W;
  AnimationSequence expected;
  UnitId            unit_id = {};
  Coord             coord   = {};

  auto f = [&] {
    return anim_seq_for_treasure_enpixelation( W.ss(), unit_id );
  };

  coord = { .x = 1, .y = 1 };
  unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord )
          .id();
  expected = {
      .sequence = {
          /*phase 0*/ {
              { .primitive =
                    P::ensure_tile_visible{ .tile = coord } } },
          /*phase 1*/ {
              { .primitive =
                    P::enpixelate_unit{ .unit_id = unit_id } },
              { .primitive = P::play_sound{
                    .what = e_sfx::treasure } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_convert_produced" ) {
  World             W;
  AnimationSequence expected;
  UnitId            unit_id   = {};
  e_direction       direction = {};
  Coord             coord     = {};

  auto f = [&] {
    return anim_seq_for_convert_produced( W.ss(), unit_id,
                                          direction );
  };

  coord = { .x = 1, .y = 1 };
  unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord )
          .id();
  direction = e_direction::w;
  expected  = {
       .sequence = {
          /*phase 0*/ {
              { .primitive =
                     P::ensure_tile_visible{ .tile = coord } },
              { .primitive =
                     P::ensure_tile_visible{
                         .tile = coord.moved( direction ) } } },
          /*phase1=*/
          { { .primitive =
                   P::enpixelate_unit{ .unit_id = unit_id } } },
          /*phase2=*/
          { { .primitive =
                   P::slide_unit{ .unit_id   = unit_id,
                                  .direction = direction } },
             { .primitive = P::play_sound{ e_sfx::move } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_colony_depixelation" ) {
  World             W;
  AnimationSequence expected;
  ColonyId          colony_id = {};

  auto f = [&] {
    return anim_seq_for_colony_depixelation( W.ss(), colony_id );
  };

  colony_id = W.add_colony( { .x = 1, .y = 0 } ).id;
  expected  = {
       .sequence = {
          /*phase 0*/ {
              { .primitive =
                     P::ensure_tile_visible{
                         .tile = { .x = 1, .y = 0 } } } },
          /*phase1=*/{
              { .primitive =
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
      .sequence = { { { .primitive = P::front_unit{
                            .unit_id = unit_id } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[anim-builders] anim_seq_for_euro_attack_euro" ) {
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
    return anim_seq_for_euro_attack_euro( W.ss(), combat );
  };

  expected = {
      .sequence = {
          /*phase 0*/ {
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 0 } } },
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 1 } } } },
          /*phase 1*/ {
              { .primitive =
                    P::front_unit{ .unit_id = defender.id() } },
              { .primitive =
                    P::slide_unit{
                        .unit_id   = attacker.id(),
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
                  P::front_unit{ .unit_id = attacker.id() } },
            { .primitive =
                  P::depixelate_euro_unit{ .unit_id =
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
                  P::pixelate_euro_unit_to_target{
                      .unit_id = attacker.id(),
                      .target  = e_unit_type::free_colonist } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id() } },
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
                  P::front_unit{ .unit_id = attacker.id() } },
            { .primitive =
                  P::depixelate_euro_unit{ .unit_id =
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
                  P::front_unit{ .unit_id = attacker.id() } },
            { .primitive =
                  P::depixelate_euro_unit{ .unit_id =
                                               defender.id() } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_won } } } );
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[anim-builders] anim_seq_for_euro_attack_brave" ) {
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
    return anim_seq_for_euro_attack_brave( W.ss(), combat );
  };

  expected = {
      .sequence = {
          /*phase 0*/ {
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 0 } } },
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 1 } } } },
          /*phase 1=*/{
              { .primitive =
                    P::front_unit{ .unit_id = defender.id } },
              { .primitive =
                    P::slide_unit{
                        .unit_id   = attacker.id(),
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
                  P::front_unit{ .unit_id = attacker.id() } },
            { .primitive =
                  P::depixelate_native_unit{ .unit_id =
                                                 defender.id } },
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
                  P::pixelate_euro_unit_to_target{
                      .unit_id = attacker.id(),
                      .target  = e_unit_type::free_colonist } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id } },
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
                  P::pixelate_euro_unit_to_target{
                      .unit_id = attacker.id(),
                      .target  = e_unit_type::free_colonist } },
            { .primitive =
                  P::pixelate_native_unit_to_target{
                      .unit_id = defender.id,
                      .target  = e_native_unit_type::
                          mounted_warrior } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_lost } } } );
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[anim-builders] anim_seq_for_brave_attack_euro" ) {
  World             W;
  AnimationSequence expected;

  Dwelling& dwelling =
      W.add_dwelling( { .x = 2, .y = 1 }, e_tribe::sioux );
  Unit const& defender = W.add_unit_on_map( e_unit_type::soldier,
                                            { .x = 1, .y = 0 } );
  NativeUnit const& attacker = W.add_native_unit_on_map(
      e_native_unit_type::mounted_brave, { .x = 1, .y = 1 },
      dwelling.id );
  CombatBraveAttackEuro combat{
      .attacker = { .id = attacker.id },
      .defender = { .id = defender.id() } };

  auto f = [&] {
    return anim_seq_for_brave_attack_euro( W.ss(), combat );
  };

  expected = {
      .sequence = {
          /*phase 0*/ {
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 1 } } },
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 0 } } } },
          /*phase 1=*/{
              { .primitive =
                    P::front_unit{ .unit_id = defender.id() } },
              { .primitive =
                    P::slide_unit{
                        .unit_id   = attacker.id,
                        .direction = e_direction::n } },
              { .primitive =
                    P::play_sound{ .what = e_sfx::move } } } } };

  SECTION( "attacker wins" ) {
    combat.winner = e_combat_winner::attacker;

    combat.attacker.outcome =
        NativeUnitCombatOutcome::no_change{};
    combat.defender.outcome = EuroUnitCombatOutcome::demoted{
        .to = e_unit_type::free_colonist };

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::front_unit{ .unit_id = attacker.id } },
            { .primitive =
                  P::pixelate_euro_unit_to_target{
                      .unit_id = defender.id(),
                      .target  = e_unit_type::free_colonist } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_won } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "defender wins" ) {
    combat.winner = e_combat_winner::defender;

    combat.attacker.outcome =
        NativeUnitCombatOutcome::destroyed{};
    combat.defender.outcome = EuroUnitCombatOutcome::no_change{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::depixelate_native_unit{ .unit_id =
                                                 attacker.id } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id() } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_lost } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "defender wins (promoted)" ) {
    combat.winner = e_combat_winner::defender;

    combat.attacker.outcome =
        NativeUnitCombatOutcome::destroyed{};
    combat.defender.outcome = EuroUnitCombatOutcome::promoted{
        .to = e_unit_type::veteran_soldier };

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::depixelate_native_unit{ .unit_id =
                                                 attacker.id } },
            { .primitive =
                  P::pixelate_euro_unit_to_target{
                      .unit_id = defender.id(),
                      .target = e_unit_type::veteran_soldier } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_lost } } } );
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[anim-builders] anim_seq_for_brave_attack_colony" ) {
  World             W;
  AnimationSequence expected;

  Unit const& defender = W.add_unit_on_map( e_unit_type::soldier,
                                            { .x = 1, .y = 0 } );
  Colony const& colony =
      W.add_colony( { .x = 1, .y = 0 }, W.default_nation() );
  Dwelling& dwelling =
      W.add_dwelling( { .x = 2, .y = 1 }, e_tribe::sioux );
  NativeUnit const& attacker = W.add_native_unit_on_map(
      e_native_unit_type::mounted_brave, { .x = 1, .y = 1 },
      dwelling.id );
  CombatBraveAttackColony combat{
      .colony_id = colony.id,
      .attacker  = { .id = attacker.id },
      .defender  = { .id = defender.id() } };

  auto f = [&] {
    return anim_seq_for_brave_attack_colony( W.ss(), combat );
  };

  expected = {
      .sequence = {
          /*phase 0*/ {
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 1 } } },
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 0 } } } },
          /*phase 1=*/{
              { .primitive =
                    P::front_unit{ .unit_id = defender.id() } },
              { .primitive =
                    P::slide_unit{
                        .unit_id   = attacker.id,
                        .direction = e_direction::n } },
              { .primitive =
                    P::play_sound{ .what = e_sfx::move } } } } };

  SECTION( "attacker wins" ) {
    combat.winner = e_combat_winner::attacker;

    combat.attacker.outcome =
        NativeUnitCombatOutcome::no_change{};
    combat.defender.outcome = EuroUnitCombatOutcome::demoted{
        .to = e_unit_type::free_colonist };

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::front_unit{ .unit_id = attacker.id } },
            { .primitive =
                  P::pixelate_euro_unit_to_target{
                      .unit_id = defender.id(),
                      .target  = e_unit_type::free_colonist } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_won } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "attacker wins (colony destroyed)" ) {
    combat.winner           = e_combat_winner::attacker;
    combat.colony_destroyed = true;

    combat.attacker.outcome =
        NativeUnitCombatOutcome::no_change{};
    combat.defender.outcome = EuroUnitCombatOutcome::no_change{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::front_unit{ .unit_id = attacker.id } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id() } },
            { .primitive =
                  P::depixelate_colony{ .colony_id =
                                            colony.id } },
            { .primitive = P::play_sound{
                  .what = e_sfx::city_destroyed } } } );
    REQUIRE( f() == expected );
  }

  SECTION(
      "attacker wins (colony destroyed) with units at gate" ) {
    combat.winner           = e_combat_winner::attacker;
    combat.colony_destroyed = true;

    UnitId const wagon_train_id =
        W.add_unit_on_map( e_unit_type::wagon_train,
                           colony.location, colony.nation )
            .id();
    UnitId const treasure_id =
        W.add_unit_on_map( e_unit_type::treasure,
                           colony.location, colony.nation )
            .id();

    combat.attacker.outcome =
        NativeUnitCombatOutcome::no_change{};
    combat.defender.outcome = EuroUnitCombatOutcome::no_change{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::front_unit{ .unit_id = attacker.id } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id() } },
            { .primitive =
                  P::hide_unit{ .unit_id = wagon_train_id } },
            { .primitive =
                  P::hide_unit{ .unit_id = treasure_id } },
            { .primitive =
                  P::depixelate_colony{ .colony_id =
                                            colony.id } },
            { .primitive = P::play_sound{
                  .what = e_sfx::city_destroyed } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "defender wins" ) {
    combat.winner = e_combat_winner::defender;

    combat.attacker.outcome =
        NativeUnitCombatOutcome::destroyed{};
    combat.defender.outcome = EuroUnitCombatOutcome::no_change{};

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::depixelate_native_unit{ .unit_id =
                                                 attacker.id } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id() } },
            { .primitive = P::play_sound{
                  .what = e_sfx::attacker_lost } } } );
    REQUIRE( f() == expected );
  }

  SECTION( "defender wins (promoted)" ) {
    combat.winner = e_combat_winner::defender;

    combat.attacker.outcome =
        NativeUnitCombatOutcome::destroyed{};
    combat.defender.outcome = EuroUnitCombatOutcome::promoted{
        .to = e_unit_type::veteran_soldier };

    expected.sequence.push_back(
        /*phase 2=*/{
            { .primitive =
                  P::depixelate_native_unit{ .unit_id =
                                                 attacker.id } },
            { .primitive =
                  P::pixelate_euro_unit_to_target{
                      .unit_id = defender.id(),
                      .target = e_unit_type::veteran_soldier } },
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
      .sequence = {
          /*phase 0*/ {
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 0, .y = 0 } } },
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 0, .y = 1 } } } },
          /*phase 1=*/{
              { .primitive =
                    P::front_unit{ .unit_id = defender.id() } },
              { .primitive =
                    P::slide_unit{
                        .unit_id   = attacker.id(),
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
                  P::front_unit{ .unit_id = attacker.id() } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id() } },
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
                            P::depixelate_euro_unit{
                                .unit_id = defender.id() } },
                      { .primitive = P::play_sound{
                            .what = e_sfx::attacker_won } } } );
    expected.sequence.push_back(
        /*phase 3=*/{
            { .primitive =
                  P::hide_unit{ .unit_id = defender.id() } },
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
                  P::depixelate_euro_unit{ .unit_id =
                                               attacker.id() } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id() } },
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
      .sequence = {
          /*phase 0*/ {
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 0 } } },
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 1 } } } },
          /*phase 1=*/{
              { .primitive =
                    P::front_unit{ .unit_id = defender.id() } },
              { .primitive =
                    P::slide_unit{
                        .unit_id   = attacker.id(),
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
                  P::front_unit{ .unit_id = attacker.id() } },
            { .primitive =
                  P::depixelate_euro_unit{ .unit_id =
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
                  P::pixelate_euro_unit_to_target{
                      .unit_id = attacker.id(),
                      .target  = e_unit_type::free_colonist } },
            { .primitive =
                  P::front_unit{ .unit_id = defender.id() } },
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
          /*phase 0*/ {
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 0 } } },
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 1 } } } },
          /*phase 1=*/
          { { .primitive =
                  P::front_unit{ .unit_id = defender_id } },
            { .primitive =
                  P::slide_unit{ .unit_id   = attacker.id(),
                                 .direction = e_direction::s } },
            { .primitive =
                  P::play_sound{ .what = e_sfx::move } } },
          /*phase 2=*/
          { { .primitive =
                  P::pixelate_euro_unit_to_target{
                      .unit_id = attacker.id(),
                      .target = e_unit_type::veteran_soldier } },
            { .primitive =
                  P::depixelate_native_unit{ .unit_id =
                                                 defender_id } },
            { .primitive =
                  P::depixelate_dwelling{ .dwelling_id =
                                              dwelling.id } },
            { .primitive =
                  P::depixelate_native_unit{
                      .unit_id = other_brave_id } },
            { .primitive =
                  P::play_sound{ .what =
                                     e_sfx::city_destroyed } },
            { .primitive =
                  P::play_sound{ .what =
                                     e_sfx::city_destroyed } },
            { .primitive = P::play_sound{
                  .what = e_sfx::city_destroyed } } } } };
  REQUIRE( f() == expected );
}

TEST_CASE(
    "[anim-builders] anim_seq_for_cheat_tribe_destruction" ) {
  World             W;
  AnimationSequence expected;
  e_tribe           tribe = {};

  W.add_tribe( e_tribe::inca ); // no dwellings.

  Dwelling const& dwelling_1 =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::apache );
  NativeUnit const& brave_1 = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 2, .y = 1 },
      dwelling_1.id );

  Dwelling const& dwelling_2 =
      W.add_dwelling( { .x = 1, .y = 2 }, e_tribe::apache );
  NativeUnit const& brave_2 = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 2, .y = 2 },
      dwelling_2.id );

  Dwelling const& dwelling_3 =
      W.add_dwelling( { .x = 1, .y = 3 }, e_tribe::aztec );
  NativeUnit const& brave_3 = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 2, .y = 3 },
      dwelling_3.id );

  Dwelling const& dwelling_4 =
      W.add_dwelling( { .x = 1, .y = 4 }, e_tribe::aztec );
  NativeUnit const& brave_4 = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 2, .y = 4 },
      dwelling_4.id );

  auto f = [&]( IVisibility const& viz ) {
    return anim_seq_for_cheat_tribe_destruction( W.ss(), viz,
                                                 tribe );
  };

  SECTION( "all visible" ) {
    VisibilityEntire const viz( W.ss() );

    tribe    = e_tribe::inca;
    expected = { .sequence = { {} } };
    REQUIRE( f( viz ) == expected );

    tribe    = e_tribe::aztec;
    expected = { .sequence = { /*phase 1=*/{
                     { .primitive =
                           P::depixelate_dwelling{
                               .dwelling_id = dwelling_3.id } },
                     { .primitive =
                           P::depixelate_dwelling{
                               .dwelling_id = dwelling_4.id } },
                     { .primitive =
                           P::depixelate_native_unit{
                               .unit_id = brave_3.id } },
                     { .primitive =
                           P::depixelate_native_unit{
                               .unit_id = brave_4.id } },
                 } } };
    REQUIRE( f( viz ) == expected );

    tribe    = e_tribe::apache;
    expected = { .sequence = { /*phase 1=*/{
                     { .primitive =
                           P::depixelate_dwelling{
                               .dwelling_id = dwelling_1.id } },
                     { .primitive =
                           P::depixelate_dwelling{
                               .dwelling_id = dwelling_2.id } },
                     { .primitive =
                           P::depixelate_native_unit{
                               .unit_id = brave_1.id } },
                     { .primitive =
                           P::depixelate_native_unit{
                               .unit_id = brave_2.id } },
                 } } };
    REQUIRE( f( viz ) == expected );
  }

  SECTION( "none visible" ) {
    VisibilityForNation const viz( W.ss(), e_nation::french );

    tribe    = e_tribe::inca;
    expected = { .sequence = { {} } };
    REQUIRE( f( viz ) == expected );

    tribe    = e_tribe::aztec;
    expected = { .sequence = { /*phase 1=*/{
                     { .primitive =
                           P::depixelate_native_unit{
                               .unit_id = brave_3.id } },
                     { .primitive =
                           P::depixelate_native_unit{
                               .unit_id = brave_4.id } },
                 } } };
    REQUIRE( f( viz ) == expected );

    tribe    = e_tribe::apache;
    expected = { .sequence = { /*phase 1=*/{
                     { .primitive =
                           P::depixelate_native_unit{
                               .unit_id = brave_1.id } },
                     { .primitive =
                           P::depixelate_native_unit{
                               .unit_id = brave_2.id } },
                 } } };
    REQUIRE( f( viz ) == expected );
  }

  SECTION( "some visible" ) {
    VisibilityForNation const viz( W.ss(), e_nation::french );

    W.map_updater().make_squares_visible(
        e_nation::french, { { .x = 1, .y = 1 } } );
    W.map_updater().make_squares_fogged(
        e_nation::french, { { .x = 1, .y = 1 } } );
    W.map_updater().make_squares_visible(
        e_nation::french, { { .x = 1, .y = 3 } } );

    tribe    = e_tribe::inca;
    expected = { .sequence = { {} } };
    REQUIRE( f( viz ) == expected );

    tribe    = e_tribe::aztec;
    expected = { .sequence = { /*phase 1=*/{
                     { .primitive =
                           P::depixelate_dwelling{
                               .dwelling_id = dwelling_3.id } },
                     { .primitive =
                           P::depixelate_native_unit{
                               .unit_id = brave_3.id } },
                     { .primitive =
                           P::depixelate_native_unit{
                               .unit_id = brave_4.id } },
                 } } };
    REQUIRE( f( viz ) == expected );

    tribe    = e_tribe::apache;
    expected = { .sequence = { /*phase 1=*/{
                     { .primitive =
                           P::depixelate_fog_dwelling{
                               .tile = { .x = 1, .y = 1 } } },
                     { .primitive =
                           P::depixelate_native_unit{
                               .unit_id = brave_1.id } },
                     { .primitive =
                           P::depixelate_native_unit{
                               .unit_id = brave_2.id } },
                 } } };
    REQUIRE( f( viz ) == expected );
  }
}

TEST_CASE( "[anim-builders] anim_seq_for_sfx" ) {
  AnimationSequence expected;
  e_sfx             sound = {};

  auto f = [&] { return anim_seq_for_sfx( sound ); };

  sound    = e_sfx::sunk_ship;
  expected = {
      .sequence = { { { .primitive = P::play_sound{
                            .what = e_sfx::sunk_ship } } } } };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
