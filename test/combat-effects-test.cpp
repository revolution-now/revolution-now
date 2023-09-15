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
#include "test/util/coro.hpp"

// Revolution Now
#include "src/damaged.rds.hpp"
#include "src/icombat.rds.hpp"
#include "src/imap-updater.hpp"
#include "src/society.rds.hpp"
#include "src/unit-mgr.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/player.rds.hpp"
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

  inline static e_nation const kAttackerNation = e_nation::dutch;
  inline static e_nation const kDefenderNation =
      e_nation::french;

  inline static e_tribe const kNativeTribe = e_tribe::sioux;

  inline static Coord const kLandAttackerCoord{ .x = 0, .y = 0 };
  inline static Coord const kLandDefenderCoord{ .x = 0, .y = 1 };
  inline static Coord const kSeaAttackerCoord{ .x = 0, .y = 2 };
  inline static Coord const kSeaDefenderCoord{ .x = 0, .y = 3 };

  inline static Coord const kNativeRaiderCoord{ .x = 2, .y = 2 };

  inline static Coord const kAttackerColonyCoord{ .x = 1,
                                                  .y = 4 };
  inline static Coord const kEuroColonyAttackerCoord{ .x = 3,
                                                      .y = 1 };
  inline static Coord const kDefenderColonyCoord{ .x = 3,
                                                  .y = 2 };
  inline static Coord const kAttackerDwellingCoord{ .x = 1,
                                                    .y = 4 };
  inline static Coord const kDefenderDwellingCoord =
      kAttackerDwellingCoord;

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, L, L, L, L, //
        L, L, L, L, L, L, L, //
        _, L, L, L, L, L, L, //
        _, L, L, L, L, L, L, //
        _, L, L, L, L, L, L, //
        _, L, L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 7 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[combat-effects] combat_effects_msg, "
    "CombatEuroAttackEuro" ) {
  World                 W;
  CombatEffectsMessages expected;

  enum class e_colony {
    // Order matters here; needs to be in order of increasing
    // visibility.
    no,
    yes_and_visible_to_owner,
    yes_and_visible_to_both,
  };

  struct Params {
    UnitType              attacker = {};
    UnitType              defender = {};
    e_combat_winner       winner   = {};
    EuroUnitCombatOutcome attacker_outcome;
    EuroUnitCombatOutcome defender_outcome;
    e_colony              attacker_colony = {};
    e_colony              defender_colony = {};
  } params;

  auto run = [&] {
    if( params.attacker_colony > e_colony::no ) {
      Colony& colony = W.add_colony( W.kAttackerColonyCoord,
                                     W.kAttackerNation );
      colony.name    = "attacker colony";
      if( params.attacker_colony >=
          e_colony::yes_and_visible_to_owner ) {
        W.map_updater().make_squares_visible(
            W.kAttackerNation, { colony.location } );
        if( params.attacker_colony >
            e_colony::yes_and_visible_to_owner ) {
          W.map_updater().make_squares_visible(
              W.kDefenderNation, { colony.location } );
        }
      }
    }
    if( params.defender_colony > e_colony::no ) {
      Colony& colony = W.add_colony( W.kDefenderColonyCoord,
                                     W.kDefenderNation );
      colony.name    = "defender colony";
      if( params.defender_colony >=
          e_colony::yes_and_visible_to_owner ) {
        W.map_updater().make_squares_visible(
            W.kDefenderNation, { colony.location } );
        if( params.defender_colony >
            e_colony::yes_and_visible_to_owner ) {
          W.map_updater().make_squares_visible(
              W.kAttackerNation, { colony.location } );
        }
      }
    }
    Unit const& attacker =
        W.add_unit_on_map( params.attacker, W.kLandAttackerCoord,
                           W.kAttackerNation );
    Unit const& defender =
        W.add_unit_on_map( params.defender, W.kLandDefenderCoord,
                           W.kDefenderNation );
    CombatEuroAttackEuro const combat{
        .winner   = params.winner,
        .attacker = { .id      = attacker.id(),
                      .outcome = params.attacker_outcome },
        .defender = { .id      = defender.id(),
                      .outcome = params.defender_outcome },
    };
    return combat_effects_msg( W.ss(), combat );
  };

  SECTION( "(soldier,soldier) -> (soldier,free_colonist)" ) {
    params = {
        .attacker         = e_unit_type::soldier,
        .defender         = e_unit_type::soldier,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome = EuroUnitCombatOutcome::demoted{
            .to = e_unit_type::free_colonist } };
    expected = {
        .summaries = { .attacker = "[Dutch] Soldier defeats "
                                   "[French] in the wilderness!",
                       .defender =
                           "[Dutch] Soldier defeats [French] in "
                           "the wilderness!" },
        .defender  = {
             .for_both = { "[French] [Soldier] routed! Unit "
                            "demoted to colonist status." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(soldier,soldier) -> (soldier,indentured_servant)" ) {
    params = {
        .attacker = e_unit_type::soldier,
        .defender =
            UnitType::create( e_unit_type::soldier,
                              e_unit_type::indentured_servant )
                .value(),
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome = EuroUnitCombatOutcome::demoted{
            .to = e_unit_type::indentured_servant } };
    expected = {
        .summaries = { .attacker = "[Dutch] Soldier defeats "
                                   "[French] in the wilderness!",
                       .defender =
                           "[Dutch] Soldier defeats [French] in "
                           "the wilderness!" },
        .defender  = {
             .for_both = { "[French] [Soldier] routed! Unit "
                            "demoted to colonist status." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(soldier,veteran_colonist) -> "
      "(soldier,veteran_colonist)" ) {
    params = {
        .attacker         = e_unit_type::soldier,
        .defender         = e_unit_type::veteran_colonist,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome =
            EuroUnitCombatOutcome::captured_and_demoted{
                .to         = e_unit_type::free_colonist,
                .new_nation = W.kAttackerNation,
                .new_coord  = W.kLandAttackerCoord } };
    expected = {
        .summaries = { .attacker = "[Dutch] Soldier defeats "
                                   "[French] in the wilderness!",
                       .defender =
                           "[Dutch] Soldier defeats [French] in "
                           "the wilderness!" },
        .defender  = {
             .for_other = { "Veteran status lost upon capture." },
             .for_both = { "[French] [Veteran Colonist] captured "
                            "by the [Dutch]!" } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(soldier,free_colonist) -> (soldier,free_colonist)" ) {
    params = {
        .attacker         = e_unit_type::soldier,
        .defender         = e_unit_type::free_colonist,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome = EuroUnitCombatOutcome::captured{
            .new_nation = W.kAttackerNation,
            .new_coord  = W.kLandAttackerCoord } };
    expected = {
        .summaries = { .attacker = "[Dutch] Soldier defeats "
                                   "[French] in the wilderness!",
                       .defender =
                           "[Dutch] Soldier defeats [French] in "
                           "the wilderness!" },
        .defender  = {
             .for_both = { "[French] [Free Colonist] captured by "
                            "the [Dutch]!" } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(soldier,artillery) -> (soldier,damaged_artillery)" ) {
    params = {
        .attacker         = e_unit_type::soldier,
        .defender         = e_unit_type::artillery,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::damaged_artillery },
        .defender_colony = e_colony::yes_and_visible_to_both };
    expected = {
        .summaries = { .attacker =
                           "[Dutch] Soldier defeats [French] "
                           "near defender colony!",
                       .defender =
                           "[Dutch] Soldier defeats [French] "
                           "near defender colony!" },
        .defender  = {
             .for_both = {
                "[French] Artillery [damaged]. Further damage "
                 "will destroy it." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(soldier,artillery) -> "
      "(veteran_soldier,damaged_artillery)" ) {
    params = {
        .attacker = e_unit_type::soldier,
        .defender = e_unit_type::artillery,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            EuroUnitCombatOutcome::promoted{
                .to = e_unit_type::veteran_soldier },
        .defender_outcome = EuroUnitCombatOutcome::demoted{
            .to = e_unit_type::damaged_artillery } };
    expected = {
        .summaries = { .attacker = "[Dutch] Soldier defeats "
                                   "[French] in the wilderness!",
                       .defender =
                           "[Dutch] Soldier defeats [French] in "
                           "the wilderness!" },
        .attacker =
            { .for_owner =
                  { "[Dutch] Soldier promoted to [Veteran "
                    "Soldier] for victory in combat!" } },
        .defender = {
            .for_both = {
                "[French] Artillery [damaged]. Further damage "
                "will destroy it." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(soldier,veteran_dragoon) -> "
      "(soldier,veteran_soldier)" ) {
    params = {
        .attacker         = e_unit_type::soldier,
        .defender         = e_unit_type::veteran_dragoon,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome = EuroUnitCombatOutcome::demoted{
            .to = e_unit_type::veteran_soldier } };
    expected = {
        .summaries = { .attacker = "[Dutch] Soldier defeats "
                                   "[French] in the wilderness!",
                       .defender =
                           "[Dutch] Soldier defeats [French] in "
                           "the wilderness!" },
        .attacker  = { .for_owner = {} },
        .defender  = {
             .for_both = {
                "[French] [Veteran Dragoon] routed! Unit "
                 "demoted to [Veteran Soldier]." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(soldier,scout) -> (veteran_soldier,)" ) {
    params = {
        .attacker = e_unit_type::soldier,
        .defender = e_unit_type::scout,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            EuroUnitCombatOutcome::promoted{
                .to = e_unit_type::veteran_soldier },
        .defender_outcome = EuroUnitCombatOutcome::destroyed{},
        .attacker_colony  = e_colony::yes_and_visible_to_owner };
    expected = {
        .summaries = { .attacker =
                           "[Dutch] Soldier defeats [French] "
                           "near attacker colony!",
                       .defender =
                           "[Dutch] Soldier defeats [French] in "
                           "the wilderness!" },
        .attacker =
            { .for_owner =
                  { "[Dutch] Soldier promoted to [Veteran "
                    "Soldier] for victory in combat!" } },
        .defender = {
            .for_owner = {
                "[French] [Scout] lost in battle." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(soldier,missionary) -> (veteran_soldier,)" ) {
    params = {
        .attacker = e_unit_type::soldier,
        .defender = e_unit_type::missionary,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            EuroUnitCombatOutcome::promoted{
                .to = e_unit_type::veteran_soldier },
        .defender_outcome = EuroUnitCombatOutcome::destroyed{},
        .attacker_colony  = e_colony::yes_and_visible_to_owner };
    expected = {
        .summaries = { .attacker =
                           "[Dutch] Soldier defeats [French] "
                           "near attacker colony!",
                       .defender =
                           "[Dutch] Soldier defeats [French] in "
                           "the wilderness!" },
        .attacker =
            { .for_owner =
                  { "[Dutch] Soldier promoted to [Veteran "
                    "Soldier] for victory in combat!" } },
        .defender = {
            .for_owner = {
                "[French] [Missionary] lost in battle." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(soldier,hardy_pioneer) -> (veteran_soldier,)" ) {
    params = {
        .attacker = e_unit_type::soldier,
        .defender = e_unit_type::hardy_pioneer,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            EuroUnitCombatOutcome::promoted{
                .to = e_unit_type::veteran_soldier },
        .defender_outcome = EuroUnitCombatOutcome::destroyed{},
        .attacker_colony  = e_colony::yes_and_visible_to_owner };
    expected = {
        .summaries = { .attacker =
                           "[Dutch] Soldier defeats [French] "
                           "near attacker colony!",
                       .defender =
                           "[Dutch] Soldier defeats [French] in "
                           "the wilderness!" },
        .attacker =
            { .for_owner =
                  { "[Dutch] Soldier promoted to [Veteran "
                    "Soldier] for victory in combat!" } },
        .defender = {
            .for_owner = {
                "[French] [Hardy Pioneer] lost in battle." } } };
    REQUIRE( run() == expected );
  }

  // NOTE: This case doesn't really happen in the game, but we
  // handle it anyway.
  SECTION(
      "(soldier,hardy_colonist) -> (soldier,free_colonist)" ) {
    params = {
        .attacker         = e_unit_type::soldier,
        .defender         = e_unit_type::hardy_colonist,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome =
            EuroUnitCombatOutcome::captured_and_demoted{
                .to         = e_unit_type::free_colonist,
                .new_nation = W.kAttackerNation,
                .new_coord  = W.kLandAttackerCoord } };
    expected = {
        .summaries = { .attacker = "[Dutch] Soldier defeats "
                                   "[French] in the wilderness!",
                       .defender =
                           "[Dutch] Soldier defeats [French] in "
                           "the wilderness!" },
        .defender  = {
             .for_other = { "Unit demoted upon capture." },
             .for_both  = { "[French] [Hardy Colonist] captured "
                              "by the [Dutch]!" } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(soldier,treasure) -> (soldier,treasure)" ) {
    params = {
        .attacker         = e_unit_type::soldier,
        .defender         = e_unit_type::treasure,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome = EuroUnitCombatOutcome::captured{
            .new_nation = W.kAttackerNation,
            .new_coord  = W.kLandAttackerCoord } };
    expected = {
        .summaries = { .attacker = "[Dutch] Soldier defeats "
                                   "[French] in the wilderness!",
                       .defender =
                           "[Dutch] Soldier defeats [French] in "
                           "the wilderness!" },
        .defender  = {
             .for_both = {
                "[French] [Treasure Train] worth [1000\x7f] "
                 "captured by the [Dutch]!" } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(soldier,treasure) -> (expert_ore_miner,treasure)" ) {
    params = {
        .attacker =
            UnitType::create( e_unit_type::soldier,
                              e_unit_type::expert_ore_miner )
                .value(),
        .defender = e_unit_type::treasure,
        .winner   = e_combat_winner::defender,
        .attacker_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::expert_ore_miner },
        .defender_outcome = EuroUnitCombatOutcome::no_change{} };
    expected = {
        .summaries = { .attacker =
                           "[French] Treasure Train defeats "
                           "[Dutch] in the wilderness!",
                       .defender =
                           "[French] Treasure Train defeats "
                           "[Dutch] in the wilderness!" },
        .attacker =
            { .for_both = { "[Dutch] [Soldier] routed! Unit "
                            "demoted to colonist status." } },
        .defender = {} };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(veteran_soldier,veteran_colonist) -> "
      "(veteran_colonist,veteran_colonist)" ) {
    params = {
        .attacker = e_unit_type::veteran_soldier,
        .defender = e_unit_type::veteran_colonist,
        .winner   = e_combat_winner::defender,
        .attacker_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::veteran_colonist },
        .defender_outcome = EuroUnitCombatOutcome::no_change{} };
    expected = {
        .summaries = { .attacker =
                           "[French] Veteran Colonist defeats "
                           "[Dutch] in the wilderness!",
                       .defender =
                           "[French] Veteran Colonist defeats "
                           "[Dutch] in the wilderness!" },
        .attacker =
            { .for_both =
                  { "[Dutch] [Veteran Soldier] routed! Unit "
                    "demoted to colonist status." } },
        .defender = {} };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(regular,free_colonist) -> (regular,free_colonist)" ) {
    params = {
        .attacker         = e_unit_type::regular,
        .defender         = e_unit_type::free_colonist,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome = EuroUnitCombatOutcome::captured{
            .new_nation = W.kAttackerNation,
            .new_coord  = W.kLandAttackerCoord } };
    expected = {
        .summaries = { .attacker = "[Dutch] Regular defeats "
                                   "[French] in the wilderness!",
                       .defender =
                           "[Dutch] Regular defeats [French] in "
                           "the wilderness!" },
        .defender  = {
             .for_both = { "[French] [Free Colonist] captured by "
                            "the [Dutch]!" } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(dragoon,native_convert) -> (dragoon,native_convert)" ) {
    params = {
        .attacker         = e_unit_type::dragoon,
        .defender         = e_unit_type::native_convert,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome = EuroUnitCombatOutcome::captured{
            .new_nation = W.kAttackerNation,
            .new_coord  = W.kLandAttackerCoord } };
    expected = {
        .summaries = { .attacker = "[Dutch] Dragoon defeats "
                                   "[French] in the wilderness!",
                       .defender =
                           "[Dutch] Dragoon defeats [French] in "
                           "the wilderness!" },
        .defender  = {
             .for_both = { "[French] [Native Convert] captured "
                            "by the [Dutch]!" } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(artillery,artillery) -> "
      "(damaged_artillery,artillery)" ) {
    params = {
        .attacker = e_unit_type::artillery,
        .defender = e_unit_type::artillery,
        .winner   = e_combat_winner::defender,
        .attacker_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::damaged_artillery },
        .defender_outcome = EuroUnitCombatOutcome::no_change{} };
    expected = {
        .summaries = { .attacker = "[French] Artillery defeats "
                                   "[Dutch] in the wilderness!",
                       .defender =
                           "[French] Artillery defeats [Dutch] "
                           "in the wilderness!" },
        .attacker  = {
             .for_both = { "[Dutch] Artillery [damaged]. Further "
                            "damage will destroy it." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(soldier,artillery) -> (petty_criminal,artillery)" ) {
    params = {
        .attacker =
            UnitType::create( e_unit_type::soldier,
                              e_unit_type::petty_criminal )
                .value(),
        .defender = e_unit_type::artillery,
        .winner   = e_combat_winner::defender,
        .attacker_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::petty_criminal },
        .defender_outcome = EuroUnitCombatOutcome::no_change{} };
    expected = {
        .summaries = { .attacker = "[French] Artillery defeats "
                                   "[Dutch] in the wilderness!",
                       .defender =
                           "[French] Artillery defeats [Dutch] "
                           "in the wilderness!" },
        .attacker  = {
             .for_both = { "[Dutch] [Soldier] routed! Unit "
                            "demoted to colonist status." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(continental_cavalry,soldier) -> "
      "(continental_army,veteran_soldier)" ) {
    params = {
        .attacker = e_unit_type::continental_cavalry,
        .defender = e_unit_type::soldier,
        .winner   = e_combat_winner::defender,
        .attacker_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::continental_army },
        .defender_outcome = EuroUnitCombatOutcome::promoted{
            .to = e_unit_type::veteran_soldier } };
    expected = {
        .summaries = { .attacker = "[French] Soldier defeats "
                                   "[Dutch] in the wilderness!",
                       .defender =
                           "[French] Soldier defeats [Dutch] in "
                           "the wilderness!" },
        .attacker =
            { .for_both =
                  { "[Dutch] [Continental Cavalry] routed! Unit "
                    "demoted to [Continental Army]." } },
        .defender = {
            .for_owner = {
                "[French] Soldier promoted to [Veteran Soldier] "
                "for victory in combat!" } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(scout,seasoned_scout) -> (,seasoned_scout)" ) {
    params = {
        .attacker         = e_unit_type::scout,
        .defender         = e_unit_type::seasoned_scout,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = EuroUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::no_change{} };
    expected = {
        .summaries = { .attacker =
                           "[French] Seasoned Scout defeats "
                           "[Dutch] in the wilderness!",
                       .defender =
                           "[French] Seasoned Scout defeats "
                           "[Dutch] in the wilderness!" },
        .attacker  = {
             .for_owner = {
                "[Dutch] [Scout] lost in battle." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(dragoon,hardy_pioneer) -> (dragoon,hardy_pioneer)" ) {
    params = {
        .attacker         = e_unit_type::dragoon,
        .defender         = e_unit_type::hardy_pioneer,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome =
            EuroUnitCombatOutcome::captured{
                .new_nation = W.kAttackerNation,
                .new_coord  = W.kLandAttackerCoord },
        .attacker_colony = e_colony::yes_and_visible_to_owner };
    expected = {
        .summaries = { .attacker =
                           "[Dutch] Dragoon defeats [French] "
                           "near attacker colony!",
                       .defender =
                           "[Dutch] Dragoon defeats [French] in "
                           "the wilderness!" },
        .defender  = {
             .for_both = { "[French] [Hardy Pioneer] captured by "
                            "the [Dutch]!" } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(dragoon,expert_fisherman) -> "
      "(dragoon,expert_fisherman)" ) {
    params = {
        .attacker         = e_unit_type::dragoon,
        .defender         = e_unit_type::expert_fisherman,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome =
            EuroUnitCombatOutcome::captured{
                .new_nation = W.kAttackerNation,
                .new_coord  = W.kLandAttackerCoord },
        .attacker_colony = e_colony::yes_and_visible_to_both };
    expected = {
        .summaries = { .attacker =
                           "[Dutch] Dragoon defeats [French] "
                           "near attacker colony!",
                       .defender =
                           "[Dutch] Dragoon defeats [French] "
                           "near attacker colony!" },
        .defender  = {
             .for_both = { "[French] [Expert Fisherman] captured "
                            "by the [Dutch]!" } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(damaged_artillery,expert_farmer) -> (,expert_farmer)" ) {
    params = {
        .attacker         = e_unit_type::damaged_artillery,
        .defender         = e_unit_type::expert_farmer,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = EuroUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::no_change{},
        .attacker_colony  = e_colony::yes_and_visible_to_owner };
    expected = {
        .summaries = { .attacker =
                           "[French] Expert Farmer defeats "
                           "[Dutch] near attacker colony!",
                       .defender =
                           "[French] Expert Farmer defeats "
                           "[Dutch] in the wilderness!" },
        .attacker  = {
             .for_both = { "Damaged [Dutch] Artillery has been "
                            "[destroyed]." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(dragoon,wagon_train) -> "
      "(veteran_dragoon,wagon_train)" ) {
    params = {
        .attacker = e_unit_type::dragoon,
        .defender = e_unit_type::wagon_train,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            EuroUnitCombatOutcome::promoted{
                .to = e_unit_type::veteran_dragoon },
        .defender_outcome = EuroUnitCombatOutcome::captured{
            .new_nation = W.kAttackerNation,
            .new_coord  = W.kLandAttackerCoord } };
    expected = {
        .summaries = { .attacker = "[Dutch] Dragoon defeats "
                                   "[French] in the wilderness!",
                       .defender =
                           "[Dutch] Dragoon defeats [French] in "
                           "the wilderness!" },
        .attacker =
            { .for_owner =
                  { "[Dutch] Dragoon promoted to [Veteran "
                    "Dragoon] for victory in combat!" } },
        .defender = {
            .for_both = { "[French] [Wagon Train] captured by "
                          "the [Dutch]!" } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(scout,wagon_train) -> (,wagon_train)" ) {
    params = {
        .attacker         = e_unit_type::scout,
        .defender         = e_unit_type::wagon_train,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = EuroUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::no_change{} };
    expected = {
        .summaries = { .attacker =
                           "[French] Wagon Train defeats "
                           "[Dutch] in the wilderness!",
                       .defender =
                           "[French] Wagon Train defeats "
                           "[Dutch] in the wilderness!" },
        .attacker  = {
             .for_owner = {
                "[Dutch] [Scout] lost in battle." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(dragoon,dragoon) -> (soldier,veteran_dragoon)" ) {
    params = {
        .attacker = e_unit_type::dragoon,
        .defender = e_unit_type::dragoon,
        .winner   = e_combat_winner::defender,
        .attacker_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::soldier },
        .defender_outcome = EuroUnitCombatOutcome::promoted{
            .to = e_unit_type::veteran_dragoon } };
    expected = {
        .summaries = { .attacker = "[French] Dragoon defeats "
                                   "[Dutch] in the wilderness!",
                       .defender =
                           "[French] Dragoon defeats [Dutch] in "
                           "the wilderness!" },
        .attacker =
            { .for_both = { "[Dutch] [Dragoon] routed! Unit "
                            "demoted to [Soldier]." } },
        .defender = {
            .for_owner = {
                "[French] Dragoon promoted to [Veteran Dragoon] "
                "for victory in combat!" } } };
    REQUIRE( run() == expected );
  }
}

TEST_CASE(
    "[combat-effects] combat_effects_msg, "
    "CombatBraveAttackEuro" ) {
  World                 W;
  CombatEffectsMessages expected;

  enum class e_colony {
    // Order matters here; needs to be in order of increasing
    // visibility.
    no,
    yes_and_visible,
  };

  struct Params {
    e_native_unit_type      attacker = {};
    UnitType                defender = {};
    e_combat_winner         winner   = {};
    NativeUnitCombatOutcome attacker_outcome;
    EuroUnitCombatOutcome   defender_outcome;
    e_colony                defender_colony = {};
  } params;

  auto run = [&] {
    if( params.defender_colony > e_colony::no ) {
      Colony& colony = W.add_colony( W.kDefenderColonyCoord,
                                     W.kDefenderNation );
      colony.name    = "defender colony";
      if( params.defender_colony >= e_colony::yes_and_visible ) {
        W.map_updater().make_squares_visible(
            W.kDefenderNation, { colony.location } );
        if( params.defender_colony >
            e_colony::yes_and_visible ) {
          W.map_updater().make_squares_visible(
              W.kAttackerNation, { colony.location } );
        }
      }
    }
    Dwelling const& dwelling = W.add_dwelling(
        W.kAttackerDwellingCoord, W.kNativeTribe );
    NativeUnit const& attacker = W.add_native_unit_on_map(
        params.attacker, W.kLandAttackerCoord, dwelling.id );
    Unit const& defender =
        W.add_unit_on_map( params.defender, W.kLandDefenderCoord,
                           W.kDefenderNation );
    CombatBraveAttackEuro const combat{
        .winner   = params.winner,
        .attacker = { .id      = attacker.id,
                      .outcome = params.attacker_outcome },
        .defender = { .id      = defender.id(),
                      .outcome = params.defender_outcome },
    };
    return combat_effects_msg( W.ss(), combat );
  };

  SECTION( "(brave,free_colonist) -> (brave,)" ) {
    params = {
        .attacker         = e_native_unit_type::brave,
        .defender         = e_unit_type::free_colonist,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = NativeUnitCombatOutcome::no_change{},
        .defender_outcome = EuroUnitCombatOutcome::destroyed{},
    };
    expected = {
        .summaries = { .defender =
                           "[Sioux] ambush [French] [Free "
                           "Colonist] in the wilderness!" },
        .defender  = {
             .for_owner = {
                "[French] [Free Colonist] lost in battle." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(brave,treasure) -> (brave,)" ) {
    params = {
        .attacker         = e_native_unit_type::brave,
        .defender         = e_unit_type::treasure,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = NativeUnitCombatOutcome::no_change{},
        .defender_outcome = EuroUnitCombatOutcome::destroyed{},
        .defender_colony  = e_colony::yes_and_visible,
    };
    expected = {
        .summaries = { .defender =
                           "[Sioux] ambush [French] [Treasure "
                           "Train] near defender colony!" },
        .defender = { .for_owner = { "[French] [Treasure Train] "
                                     "lost in battle." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(brave,wagon_train) -> (brave,)" ) {
    params = {
        .attacker         = e_native_unit_type::brave,
        .defender         = e_unit_type::wagon_train,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = NativeUnitCombatOutcome::no_change{},
        .defender_outcome = EuroUnitCombatOutcome::destroyed{},
    };
    expected = {
        .summaries = { .defender =
                           "[Sioux] ambush [French] [Wagon "
                           "Train] in the wilderness!" },
        .defender  = {
             .for_owner = {
                "[French] [Wagon Train] lost in battle." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(brave,dragoon) -> (mounted_brave,soldier)" ) {
    params = {
        .attacker = e_native_unit_type::brave,
        .defender = e_unit_type::dragoon,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            NativeUnitCombatOutcome::promoted{
                .to = e_native_unit_type::mounted_brave },
        .defender_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::soldier },
    };
    expected = {
        .summaries = { .defender =
                           "[Sioux] ambush [French] [Dragoon] "
                           "in the wilderness!" },
        .attacker =
            { .for_both =
                  { "[Sioux] Braves have acquired [horses] upon "
                    "victory in combat!" } },
        .defender = {
            .for_both = { "[French] [Dragoon] routed! Unit "
                          "demoted to [Soldier]." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(brave,veteran_soldier) -> "
      "(armed_brave,veteran_colonist)" ) {
    params = {
        .attacker = e_native_unit_type::brave,
        .defender = e_unit_type::veteran_soldier,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            NativeUnitCombatOutcome::promoted{
                .to = e_native_unit_type::armed_brave },
        .defender_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::veteran_colonist },
    };
    expected = {
        .summaries = { .defender =
                           "[Sioux] ambush [French] [Veteran "
                           "Soldier] in the wilderness!" },
        .attacker =
            { .for_both =
                  { "[Sioux] Braves have acquired [muskets] "
                    "upon victory in combat!" } },
        .defender = {
            .for_both = {
                "[French] [Veteran Soldier] routed! Unit "
                "demoted to colonist status." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(mounted_brave,soldier) -> "
      "(mounted_warrior,free_colonist)" ) {
    params = {
        .attacker = e_native_unit_type::mounted_brave,
        .defender = e_unit_type::soldier,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            NativeUnitCombatOutcome::promoted{
                .to = e_native_unit_type::mounted_warrior },
        .defender_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::free_colonist },
    };
    expected = {
        .summaries = { .defender =
                           "[Sioux] ambush [French] [Soldier] "
                           "in the wilderness!" },
        .attacker =
            { .for_both =
                  { "[Sioux] Mounted Braves have acquired "
                    "[muskets] upon victory in combat!" } },
        .defender = {
            .for_both = { "[French] [Soldier] routed! Unit "
                          "demoted to colonist status." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(armed_brave,dragoon) -> (mounted_warrior,soldier)" ) {
    params = {
        .attacker = e_native_unit_type::armed_brave,
        .defender = e_unit_type::dragoon,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            NativeUnitCombatOutcome::promoted{
                .to = e_native_unit_type::mounted_warrior },
        .defender_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::soldier },
    };
    expected = {
        .summaries = { .defender =
                           "[Sioux] ambush [French] [Dragoon] "
                           "in the wilderness!" },
        .attacker =
            { .for_both =
                  { "[Sioux] Armed Braves have acquired "
                    "[horses] upon victory in combat!" } },
        .defender = {
            .for_both = { "[French] [Dragoon] routed! Unit "
                          "demoted to [Soldier]." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(mounted_brave,artillery) -> "
      "(mounted_brave,damaged_artillery)" ) {
    params = {
        .attacker         = e_native_unit_type::mounted_brave,
        .defender         = e_unit_type::artillery,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = NativeUnitCombatOutcome::no_change{},
        .defender_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::damaged_artillery },
    };
    expected = {
        .summaries = { .defender =
                           "[Sioux] ambush [French] [Artillery] "
                           "in the wilderness!" },
        .defender  = {
             .for_both = {
                "[French] Artillery [damaged]. Further damage "
                 "will destroy it." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(brave,free_colonist) -> (,free_colonist)" ) {
    params = {
        .attacker         = e_native_unit_type::brave,
        .defender         = e_unit_type::free_colonist,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::no_change{},
    };
    expected = { .summaries = {
                     .defender =
                         "[French] Free Colonist defeats "
                         "[Sioux] Brave in the wilderness!" } };
    REQUIRE( run() == expected );
  }

  SECTION( "(armed_brave,soldier) -> (,soldier)" ) {
    params = {
        .attacker         = e_native_unit_type::armed_brave,
        .defender         = e_unit_type::soldier,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::no_change{},
    };
    expected = {
        .summaries = { .defender =
                           "[French] Soldier defeats [Sioux] "
                           "Armed Brave in the wilderness!" } };
    REQUIRE( run() == expected );
  }

  SECTION( "(mounted_brave,soldier) -> (,soldier)" ) {
    params = {
        .attacker         = e_native_unit_type::mounted_brave,
        .defender         = e_unit_type::soldier,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::no_change{},
    };
    expected = { .summaries = {
                     .defender =
                         "[French] Soldier defeats [Sioux] "
                         "Mounted Brave in the wilderness!" } };
    REQUIRE( run() == expected );
  }

  SECTION( "(mounted_warrior,soldier) -> (,veteran_soldier)" ) {
    params = {
        .attacker         = e_native_unit_type::mounted_brave,
        .defender         = e_unit_type::soldier,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome =
            EuroUnitCombatOutcome::promoted{
                .to = e_unit_type::veteran_soldier },
    };
    expected = {
        .summaries = { .defender =
                           "[French] Soldier defeats [Sioux] "
                           "Mounted Brave in the wilderness!" },
        .defender  = {
             .for_owner = {
                "[French] Soldier promoted to [Veteran Soldier] "
                 "for victory in combat!" } } };
    REQUIRE( run() == expected );
  }
}

TEST_CASE(
    "[combat-effects] combat_effects_msg, "
    "CombatShipAttackShip" ) {
  World                 W;
  CombatEffectsMessages expected;

  enum class e_colony {
    // Order matters here; needs to be in order of increasing
    // visibility.
    no,
    yes_and_visible_to_owner,
    yes_and_visible_to_both,
  };

  struct Params {
    e_unit_type                attacker = {};
    e_unit_type                defender = {};
    maybe<e_combat_winner>     winner   = {};
    EuroNavalUnitCombatOutcome attacker_outcome;
    EuroNavalUnitCombatOutcome defender_outcome;
    e_colony                   attacker_colony   = {};
    e_colony                   defender_colony   = {};
    int                        units_on_attacker = 0;
    int                        units_on_defender = 0;
  } params;

  auto run = [&] {
    if( params.attacker_colony > e_colony::no ) {
      Colony& colony = W.add_colony( W.kAttackerColonyCoord,
                                     W.kAttackerNation );
      colony.name    = "attacker colony";
      if( params.attacker_colony >=
          e_colony::yes_and_visible_to_owner ) {
        W.map_updater().make_squares_visible(
            W.kAttackerNation, { colony.location } );
        if( params.attacker_colony >
            e_colony::yes_and_visible_to_owner ) {
          W.map_updater().make_squares_visible(
              W.kDefenderNation, { colony.location } );
        }
      }
    }
    if( params.defender_colony > e_colony::no ) {
      Colony& colony = W.add_colony( W.kDefenderColonyCoord,
                                     W.kDefenderNation );
      colony.name    = "defender colony";
      if( params.defender_colony >=
          e_colony::yes_and_visible_to_owner ) {
        W.map_updater().make_squares_visible(
            W.kDefenderNation, { colony.location } );
        if( params.defender_colony >
            e_colony::yes_and_visible_to_owner ) {
          W.map_updater().make_squares_visible(
              W.kAttackerNation, { colony.location } );
        }
      }
    }
    Unit const& attacker =
        W.add_unit_on_map( params.attacker, W.kSeaAttackerCoord,
                           W.kAttackerNation );
    for( int i = 0; i < params.units_on_attacker; ++i )
      W.add_unit_in_cargo( e_unit_type::free_colonist,
                           attacker.id() );
    Unit const& defender =
        W.add_unit_on_map( params.defender, W.kSeaDefenderCoord,
                           W.kDefenderNation );
    for( int i = 0; i < params.units_on_defender; ++i )
      W.add_unit_in_cargo( e_unit_type::free_colonist,
                           defender.id() );
    CombatShipAttackShip const combat{
        .winner   = params.winner,
        .attacker = { .id      = attacker.id(),
                      .outcome = params.attacker_outcome },
        .defender = { .id      = defender.id(),
                      .outcome = params.defender_outcome },
    };
    return combat_effects_msg( W.ss(), combat );
  };

  SECTION( "(privateer,caravel) -> (privateer,[damaged])" ) {
    params = {
        .attacker = e_unit_type::privateer,
        .defender = e_unit_type::caravel,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            EuroNavalUnitCombatOutcome::no_change{},
        .defender_outcome = EuroNavalUnitCombatOutcome::damaged{
            .port = ShipRepairPort::european_harbor{} } };
    expected = {
        .summaries =
            { .attacker =
                  "[Dutch] Privateer defeats [French] at sea!",
              .defender =
                  "[Dutch] Privateer defeats [French] at sea!" },
        .defender = {
            .for_both = {
                "[French] [Caravel] damaged in battle! Ship "
                "sent to [La Rochelle] for repairs." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(privateer,caravel[2]) -> (privateer,[damaged])" ) {
    params = {
        .attacker = e_unit_type::privateer,
        .defender = e_unit_type::caravel,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            EuroNavalUnitCombatOutcome::no_change{},
        .defender_outcome =
            EuroNavalUnitCombatOutcome::damaged{
                .port = ShipRepairPort::european_harbor{} },
        .units_on_defender = 2 };
    expected = {
        .summaries =
            { .attacker =
                  "[Dutch] Privateer defeats [French] at sea!",
              .defender =
                  "[Dutch] Privateer defeats [French] at sea!" },
        .defender = {
            .for_owner =
                { "[Two] units onboard have been lost." },
            .for_both = {
                "[French] [Caravel] damaged in battle! Ship "
                "sent to [La Rochelle] for repairs." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(privateer,caravel) -> (privateer,[evade])" ) {
    params   = { .attacker = e_unit_type::privateer,
                 .defender = e_unit_type::caravel,
                 .winner   = nothing,
                 .attacker_outcome =
                     EuroNavalUnitCombatOutcome::no_change{},
                 .defender_outcome =
                     EuroNavalUnitCombatOutcome::no_change{} };
    expected = {
        .defender = { .for_both = { "[French] [Caravel] evades "
                                    "[Dutch] [Privateer]." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(privateer,frigate) -> (privateer,[sunk])" ) {
    params = {
        .attacker = e_unit_type::privateer,
        .defender = e_unit_type::frigate,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            EuroNavalUnitCombatOutcome::moved{
                .to = W.kSeaDefenderCoord },
        .defender_outcome = EuroNavalUnitCombatOutcome::sunk{} };
    expected = {
        .summaries = { .attacker = { "[Dutch] Privateer defeats "
                                     "[French] at sea!" },
                       .defender = { "[Dutch] Privateer defeats "
                                     "[French] at sea!" } },
        .defender = { .for_both = { "[French] [Frigate] sunk by "
                                    "[Dutch] [Privateer]." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(privateer,frigate[1]) -> (privateer,[sunk])" ) {
    params = {
        .attacker = e_unit_type::privateer,
        .defender = e_unit_type::frigate,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            EuroNavalUnitCombatOutcome::moved{
                .to = W.kSeaDefenderCoord },
        .defender_outcome  = EuroNavalUnitCombatOutcome::sunk{},
        .units_on_defender = 1 };
    expected = {
        .summaries = { .attacker = { "[Dutch] Privateer defeats "
                                     "[French] at sea!" },
                       .defender = { "[Dutch] Privateer defeats "
                                     "[French] at sea!" } },
        .defender  = {
             .for_owner = { "[One] unit onboard has been lost." },
             .for_both  = { "[French] [Frigate] sunk by "
                              "[Dutch] [Privateer]." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(privateer[2],caravel) -> ([damaged],caravel)" ) {
    params = {
        .attacker = e_unit_type::privateer,
        .defender = e_unit_type::caravel,
        .winner   = e_combat_winner::defender,
        .attacker_outcome =
            EuroNavalUnitCombatOutcome::damaged{
                .port = ShipRepairPort::european_harbor{} },
        .defender_outcome =
            EuroNavalUnitCombatOutcome::no_change{},
        .defender_colony   = e_colony::yes_and_visible_to_owner,
        .units_on_attacker = 2 };
    expected = {
        .summaries =
            { .attacker =
                  "[French] Caravel defeats [Dutch] at sea!",
              .defender = "[French] Caravel defeats [Dutch] "
                          "near defender colony!" },
        .attacker = {
            .for_owner =
                { "[Two] units onboard have been lost." },
            .for_both = {
                "[Dutch] [Privateer] damaged in battle! Ship "
                "sent to [Amsterdam] for repairs." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(privateer[2],caravel) -> ([damaged/drydock],caravel)" ) {
    params = {
        .attacker = e_unit_type::privateer,
        .defender = e_unit_type::caravel,
        .winner   = e_combat_winner::defender,
        .attacker_outcome =
            EuroNavalUnitCombatOutcome::damaged{
                .port =
                    ShipRepairPort::colony{
                        .id = ColonyId{ 1 } } },
        .defender_outcome =
            EuroNavalUnitCombatOutcome::no_change{},
        .defender_colony   = e_colony::yes_and_visible_to_owner,
        .units_on_attacker = 2 };
    expected = {
        .summaries =
            { .attacker =
                  "[French] Caravel defeats [Dutch] at sea!",
              .defender = "[French] Caravel defeats [Dutch] "
                          "near defender colony!" },
        .attacker = {
            .for_owner =
                { "[Two] units onboard have been lost." },
            .for_both = {
                "[Dutch] [Privateer] damaged in battle! Ship "
                "sent to [defender colony] for repairs." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(privateer[1],frigate) -> ([sunk],frigate)" ) {
    params = {
        .attacker         = e_unit_type::privateer,
        .defender         = e_unit_type::frigate,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = EuroNavalUnitCombatOutcome::sunk{},
        .defender_outcome =
            EuroNavalUnitCombatOutcome::no_change{},
        .attacker_colony   = e_colony::yes_and_visible_to_both,
        .units_on_attacker = 2 };
    expected = {
        .summaries =
            { .attacker = { "[French] Frigate defeats [Dutch] "
                            "near attacker colony!" },
              .defender = { "[French] Frigate defeats [Dutch] "
                            "near attacker colony!" } },
        .attacker = {
            .for_owner =
                { "[Two] units onboard have been lost." },
            .for_both = { "[Dutch] [Privateer] sunk by [French] "
                          "[Frigate]." } } };
    REQUIRE( run() == expected );
  }
}

TEST_CASE(
    "[combat-effects] combat_effects_msg, "
    "CombatColonyArtilleryAttackShip" ) {
  World                 W;
  CombatEffectsMessages expected;

  struct Params {
    e_unit_type                  defender = {};
    e_combat_winner              winner   = {};
    ColonyArtilleryCombatOutcome attacker_outcome;
    EuroNavalUnitCombatOutcome   defender_outcome;
    int                          units_on_defender = 0;
    e_colony_building            building          = {};
  } params;

  // Make sure that the defending ship is adjacent to the attack-
  // er's colony. Probably not necessary for this test, but just
  // in case, since that's how it'd be in the real game.
  BASE_CHECK(
      W.kAttackerColonyCoord.direction_to( W.kSeaDefenderCoord )
          .has_value() );

  Colony& colony =
      W.add_colony( W.kAttackerColonyCoord, W.kAttackerNation );
  colony.name = "colony-with-guns";
  // In a real situation the colony will always be visible and
  // unfogged to both nations, since they are adjacent.
  W.map_updater().make_squares_visible( W.kAttackerNation,
                                        { colony.location } );
  W.map_updater().make_squares_visible( W.kDefenderNation,
                                        { colony.location } );

  auto run = [&] {
    colony.buildings[params.building] = true;
    Unit const& defender =
        W.add_unit_on_map( params.defender, W.kSeaDefenderCoord,
                           W.kDefenderNation );
    for( int i = 0; i < params.units_on_defender; ++i )
      W.add_unit_in_cargo( e_unit_type::free_colonist,
                           defender.id() );
    CombatColonyArtilleryAttackShip const combat{
        .winner   = params.winner,
        .attacker = { .id      = colony.id,
                      .outcome = params.attacker_outcome },
        .defender = { .id      = defender.id(),
                      .outcome = params.defender_outcome },
    };
    return combat_effects_msg( W.ss(), combat );
  };

  SECTION( "(fort,caravel) -> (fort,[damaged])" ) {
    params = {
        .defender         = e_unit_type::caravel,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = ColonyArtilleryCombatOutcome::win{},
        .defender_outcome =
            EuroNavalUnitCombatOutcome::damaged{
                .port = ShipRepairPort::european_harbor{} },
        .building = e_colony_building::fort };
    expected = {
        .summaries =
            { .attacker =
                  "[Dutch] coastal fortification defeats "
                  "[French] in colony-with-guns!",
              .defender =
                  "[Dutch] coastal fortification defeats "
                  "[French] in colony-with-guns!" },
        .defender = {
            .for_both = {
                "[French] [Caravel] damaged in battle! Ship "
                "sent to [La Rochelle] for repairs." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(fortress,caravel[2]) -> (fortress,[damaged])" ) {
    params = {
        .defender         = e_unit_type::caravel,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = ColonyArtilleryCombatOutcome::win{},
        .defender_outcome =
            EuroNavalUnitCombatOutcome::damaged{
                .port = ShipRepairPort::european_harbor{} },
        .units_on_defender = 2,
        .building          = e_colony_building::fortress };
    expected = {
        .summaries =
            { .attacker =
                  "[Dutch] coastal fortification defeats "
                  "[French] in colony-with-guns!",
              .defender =
                  "[Dutch] coastal fortification defeats "
                  "[French] in colony-with-guns!" },
        .defender = {
            .for_owner =
                { "[Two] units onboard have been lost." },
            .for_both = {
                "[French] [Caravel] damaged in battle! Ship "
                "sent to [La Rochelle] for repairs." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(fort,frigate) -> (fort,[sunk])" ) {
    params = {
        .defender         = e_unit_type::frigate,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = ColonyArtilleryCombatOutcome::win{},
        .defender_outcome = EuroNavalUnitCombatOutcome::sunk{},
        .building         = e_colony_building::fort };
    expected = {
        .summaries =
            { .attacker =
                  "[Dutch] coastal fortification defeats "
                  "[French] in colony-with-guns!",
              .defender =
                  "[Dutch] coastal fortification defeats "
                  "[French] in colony-with-guns!" },
        .defender = { .for_both = { "[French] [Frigate] sunk by "
                                    "[Dutch] [Fort]." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(fortress,frigate[1]) -> (fortress,[sunk])" ) {
    params = {
        .defender          = e_unit_type::frigate,
        .winner            = e_combat_winner::attacker,
        .attacker_outcome  = ColonyArtilleryCombatOutcome::win{},
        .defender_outcome  = EuroNavalUnitCombatOutcome::sunk{},
        .units_on_defender = 1,
        .building          = e_colony_building::fortress };
    expected = {
        .summaries =
            { .attacker =
                  "[Dutch] coastal fortification defeats "
                  "[French] in colony-with-guns!",
              .defender =
                  "[Dutch] coastal fortification defeats "
                  "[French] in colony-with-guns!" },
        .defender = {
            .for_owner = { "[One] unit onboard has been lost." },
            .for_both  = { "[French] [Frigate] sunk by [Dutch] "
                            "[Fortress]." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(fort,caravel) -> (fort,caravel)" ) {
    params = {
        .defender         = e_unit_type::caravel,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = ColonyArtilleryCombatOutcome::lose{},
        .defender_outcome =
            EuroNavalUnitCombatOutcome::no_change{},
        .building = e_colony_building::fort };
    expected = {
        .summaries = { .attacker =
                           "[French] Caravel defeats [Dutch] in "
                           "colony-with-guns!",
                       .defender =
                           "[French] Caravel defeats [Dutch] in "
                           "colony-with-guns!" },
    };
    REQUIRE( run() == expected );
  }

  SECTION( "(fortress,privateer) -> (fortress,privateer)" ) {
    params = {
        .defender         = e_unit_type::privateer,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = ColonyArtilleryCombatOutcome::lose{},
        .defender_outcome =
            EuroNavalUnitCombatOutcome::no_change{},
        .building = e_colony_building::fortress };
    expected = {
        .summaries = { .attacker =
                           "[French] Privateer defeats [Dutch] "
                           "in colony-with-guns!",
                       .defender =
                           "[French] Privateer defeats [Dutch] "
                           "in colony-with-guns!" },
    };
    REQUIRE( run() == expected );
  }
}

TEST_CASE(
    "[combat-effects] combat_effects_msg, "
    "CombatBraveAttackColony" ) {
  World                 W;
  CombatEffectsMessages expected;

  struct Params {
    e_native_unit_type      attacker         = {};
    maybe<e_unit_type>      defender_at_gate = {};
    e_combat_winner         winner           = {};
    NativeUnitCombatOutcome attacker_outcome;
    EuroUnitCombatOutcome   defender_outcome;
    bool                    burned = false;
  } params;

  BASE_CHECK(
      W.kNativeRaiderCoord.direction_to( W.kDefenderColonyCoord )
          .has_value() );
  auto [colony, founder] = W.found_colony_with_new_unit(
      W.kDefenderColonyCoord, W.kDefenderNation );
  colony.name = "raided-colony";

  auto run = [&] {
    Dwelling const& dwelling = W.add_dwelling(
        W.kAttackerDwellingCoord, W.kNativeTribe );
    NativeUnit const& attacker = W.add_native_unit_on_map(
        params.attacker, W.kNativeRaiderCoord, dwelling.id );
    Unit const& defender =
        params.defender_at_gate.has_value()
            ? W.add_unit_on_map( *params.defender_at_gate,
                                 W.kLandDefenderCoord,
                                 W.kDefenderNation )
            : founder;
    CombatBraveAttackColony const combat{
        .winner           = params.winner,
        .colony_id        = colony.id,
        .colony_destroyed = params.burned,
        .attacker         = { .id      = attacker.id,
                              .outcome = params.attacker_outcome },
        .defender         = { .id      = defender.id(),
                              .outcome = params.defender_outcome },
    };
    return combat_effects_msg( W.ss(), combat );
  };

  SECTION( "no units at gate, not burned" ) {
    params = {
        .attacker         = e_native_unit_type::brave,
        .defender_at_gate = nothing,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::no_change{},
        .burned           = false };
    expected = {
        .summaries = {
            .defender =
                "[Sioux] raiding party wiped out in "
                "[raided-colony]! Colonists celebrate!" } };
    REQUIRE( run() == expected );
  }

  SECTION( "non-military unit at gate, not burned" ) {
    params = {
        .attacker         = e_native_unit_type::brave,
        .defender_at_gate = e_unit_type::expert_farmer,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::no_change{},
        .burned           = false };
    expected = {
        .summaries = {
            .defender =
                "[Sioux] raiding party wiped out in "
                "[raided-colony]! Colonists celebrate!" } };
    REQUIRE( run() == expected );
  }

  SECTION( "military unit at gate, defender wins" ) {
    params = {
        .attacker         = e_native_unit_type::brave,
        .defender_at_gate = e_unit_type::soldier,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::no_change{},
        .burned           = false };
    expected = {
        .summaries = {
            .defender =
                "[Sioux] raiding party wiped out in "
                "[raided-colony]! Colonists celebrate!" } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "military unit at gate, defender wins w/ promotion" ) {
    params = {
        .attacker         = e_native_unit_type::brave,
        .defender_at_gate = e_unit_type::soldier,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome =
            EuroUnitCombatOutcome::promoted{
                .to = e_unit_type::veteran_soldier },
        .burned = false };
    expected = {
        .summaries =
            { .defender =
                  "[Sioux] raiding party wiped out in "
                  "[raided-colony]! Colonists celebrate!" },
        .defender = {
            .for_owner = {
                "[French] Soldier promoted to [Veteran Soldier] "
                "for victory in combat!" } } };
    REQUIRE( run() == expected );
  }

  SECTION( "military unit at gate, defender loses" ) {
    params = {
        .attacker         = e_native_unit_type::brave,
        .defender_at_gate = e_unit_type::soldier,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::free_colonist },
        .burned = false };
    expected = {
        .summaries = { .defender =
                           "[Sioux] ambush [French] [Soldier] "
                           "in [raided-colony]!" },
        .defender  = {
             .for_both = { "[French] [Soldier] routed! Unit "
                            "demoted to colonist status." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "military unit (scout) at gate, defender loses" ) {
    params = {
        .attacker         = e_native_unit_type::brave,
        .defender_at_gate = e_unit_type::scout,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::destroyed{},
        .burned           = false };
    expected = {
        .summaries = { .defender =
                           "[Sioux] ambush [French] [Scout] in "
                           "[raided-colony]!" },
        .defender  = {
             .for_owner = {
                "[French] [Scout] lost in battle." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "no units at gate, burned" ) {
    params = {
        .attacker         = e_native_unit_type::brave,
        .defender_at_gate = nothing,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::destroyed{},
        .burned           = true };
    expected = { .summaries = {
                     .defender =
                         "[Sioux] massacre [French] population "
                         "in [raided-colony]! Colony set ablaze "
                         "and decimated! The King demands "
                         "accountability!" } };
    REQUIRE( run() == expected );
  }

  SECTION( "non-military unit at gate, burned" ) {
    params = {
        .attacker         = e_native_unit_type::brave,
        .defender_at_gate = e_unit_type::expert_farmer,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::destroyed{},
        .burned           = true };
    expected = { .summaries = {
                     .defender =
                         "[Sioux] massacre [French] population "
                         "in [raided-colony]! Colony set ablaze "
                         "and decimated! The King demands "
                         "accountability!" } };
    REQUIRE( run() == expected );
  }

  SECTION( "no units at gate, burned, after revolution" ) {
    W.french().revolution_status = e_revolution_status::declared;

    params = {
        .attacker         = e_native_unit_type::brave,
        .defender_at_gate = nothing,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = NativeUnitCombatOutcome::destroyed{},
        .defender_outcome = EuroUnitCombatOutcome::destroyed{},
        .burned           = true };
    expected = { .summaries = {
                     .defender =
                         "[Sioux] massacre [French] population "
                         "in [raided-colony]! Colony set ablaze "
                         "and decimated! The King laughs at "
                         "such incompetent governance!" } };
    REQUIRE( run() == expected );
  }
}

TEST_CASE(
    "[combat-effects] combat_effects_msg, "
    "CombatEuroAttackBrave" ) {
  World                 W;
  CombatEffectsMessages expected;

  enum class e_colony {
    // Order matters here; needs to be in order of increasing
    // visibility.
    no,
    yes_and_visible,
  };

  struct Params {
    UnitType                attacker = {};
    e_native_unit_type      defender = {};
    e_combat_winner         winner   = {};
    EuroUnitCombatOutcome   attacker_outcome;
    NativeUnitCombatOutcome defender_outcome;
    e_colony                attacker_colony = {};
  } params;

  auto run = [&] {
    if( params.attacker_colony > e_colony::no ) {
      Colony& colony = W.add_colony( W.kAttackerColonyCoord,
                                     W.kAttackerNation );
      colony.name    = "attacker colony";
      if( params.attacker_colony >= e_colony::yes_and_visible ) {
        W.map_updater().make_squares_visible(
            W.kAttackerNation, { colony.location } );
        if( params.attacker_colony >
            e_colony::yes_and_visible ) {
          W.map_updater().make_squares_visible(
              W.kAttackerNation, { colony.location } );
        }
      }
    }
    Dwelling const& dwelling = W.add_dwelling(
        W.kDefenderDwellingCoord, W.kNativeTribe );
    NativeUnit const& defender = W.add_native_unit_on_map(
        params.defender, W.kLandDefenderCoord, dwelling.id );
    Unit const& attacker =
        W.add_unit_on_map( params.attacker, W.kLandAttackerCoord,
                           W.kAttackerNation );
    CombatEuroAttackBrave const combat{
        .winner   = params.winner,
        .attacker = { .id      = attacker.id(),
                      .outcome = params.attacker_outcome },
        .defender = { .id      = defender.id,
                      .outcome = params.defender_outcome },
    };
    return combat_effects_msg( W.ss(), combat );
  };

  SECTION( "(soldier,brave) -> (soldier,)" ) {
    params = {
        .attacker         = e_unit_type::soldier,
        .defender         = e_native_unit_type::brave,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome = NativeUnitCombatOutcome::destroyed{},
    };
    expected = {
        .summaries = { .attacker =
                           "[Dutch] Soldier defeats [Sioux] "
                           "Brave in the wilderness!" } };
    REQUIRE( run() == expected );
  }

  SECTION( "(soldier,mounted_brave) -> (veteran_soldier,)" ) {
    params = {
        .attacker = e_unit_type::soldier,
        .defender = e_native_unit_type::mounted_brave,
        .winner   = e_combat_winner::attacker,
        .attacker_outcome =
            EuroUnitCombatOutcome::promoted{
                .to = e_unit_type::veteran_soldier },
        .defender_outcome = NativeUnitCombatOutcome::destroyed{},
    };
    expected = {
        .summaries = { .attacker =
                           "[Dutch] Soldier defeats [Sioux] "
                           "Mounted Brave in the wilderness!" },
        .attacker  = {
             .for_owner = {
                "[Dutch] Soldier promoted to [Veteran Soldier] "
                 "for victory in combat!" } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(soldier,brave) -> (free_colonist,armed_brave)" ) {
    params   = { .attacker = e_unit_type::soldier,
                 .defender = e_native_unit_type::brave,
                 .winner   = e_combat_winner::defender,
                 .attacker_outcome =
                     EuroUnitCombatOutcome::demoted{
                         .to = e_unit_type::free_colonist },
                 .defender_outcome =
                     NativeUnitCombatOutcome::promoted{
                         .to = e_native_unit_type::armed_brave },
                 .attacker_colony = e_colony::yes_and_visible };
    expected = {
        .summaries = { .attacker =
                           "[Sioux] defeat [Dutch] [Soldier] "
                           "near attacker colony!" },
        .attacker =
            { .for_both = { "[Dutch] [Soldier] routed! Unit "
                            "demoted to colonist status." } },
        .defender =
            { .for_both =
                  { "[Sioux] Braves have acquired [muskets] "
                    "upon victory in combat!" } },
    };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(continental_cavalry,armed_brave) -> "
      "(continental_army,mounted_warrior)" ) {
    params = {
        .attacker = e_unit_type::continental_cavalry,
        .defender = e_native_unit_type::armed_brave,
        .winner   = e_combat_winner::defender,
        .attacker_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::continental_army },
        .defender_outcome = NativeUnitCombatOutcome::promoted{
            .to = e_native_unit_type::mounted_warrior } };
    expected = {
        .summaries = { .attacker =
                           "[Sioux] defeat [Dutch] [Continental "
                           "Cavalry] in the wilderness!" },
        .attacker =
            { .for_both =
                  { "[Dutch] [Continental Cavalry] routed! Unit "
                    "demoted to [Continental Army]." } },
        .defender =
            { .for_both =
                  { "[Sioux] Armed Braves have acquired "
                    "[horses] upon victory in combat!" } },
    };
    REQUIRE( run() == expected );
  }

  // NOTE: in the OG, braves capture horses when attacking and
  // defeating a scout.
  SECTION( "(seasoned_scout,brave) -> (,mounted_brave)" ) {
    params = {
        .attacker         = e_unit_type::seasoned_scout,
        .defender         = e_native_unit_type::brave,
        .winner           = e_combat_winner::defender,
        .attacker_outcome = EuroUnitCombatOutcome::destroyed{},
        .defender_outcome = NativeUnitCombatOutcome::promoted{
            .to = e_native_unit_type::mounted_brave } };
    expected = {
        .summaries = { .attacker =
                           "[Sioux] defeat [Dutch] [Seasoned "
                           "Scout] in the wilderness!" },
        .attacker  = { .for_owner = { "[Dutch] [Seasoned Scout] "
                                       "lost in battle." } },
        .defender =
            { .for_both =
                  { "[Sioux] Braves have acquired [horses] upon "
                    "victory in combat!" } },
    };
    REQUIRE( run() == expected );
  }
}

TEST_CASE(
    "[combat-effects] combat_effects_msg, "
    "CombatEuroAttackDwelling" ) {
  World                 W;
  CombatEffectsMessages expected;

  enum class e_colony {
    // Order matters here; needs to be in order of increasing
    // visibility.
    no,
    yes_and_visible,
  };

  struct Params {
    UnitType              attacker = {};
    e_combat_winner       winner   = {};
    EuroUnitCombatOutcome attacker_outcome;
    DwellingCombatOutcome defender_outcome;
    e_colony              attacker_colony = {};
  } params;

  auto run = [&] {
    if( params.attacker_colony > e_colony::no ) {
      Colony& colony = W.add_colony( W.kAttackerColonyCoord,
                                     W.kAttackerNation );
      colony.name    = "attacker colony";
      if( params.attacker_colony >= e_colony::yes_and_visible ) {
        W.map_updater().make_squares_visible(
            W.kAttackerNation, { colony.location } );
        if( params.attacker_colony >
            e_colony::yes_and_visible ) {
          W.map_updater().make_squares_visible(
              W.kAttackerNation, { colony.location } );
        }
      }
    }
    Dwelling const& dwelling = W.add_dwelling(
        W.kDefenderDwellingCoord, W.kNativeTribe );
    Unit const& attacker =
        W.add_unit_on_map( params.attacker, W.kLandAttackerCoord,
                           W.kAttackerNation );
    CombatEuroAttackDwelling const combat{
        // Here we ignore the other fields because messages asso-
        // ciated with those are generated in another module.
        .winner   = params.winner,
        .attacker = { .id      = attacker.id(),
                      .outcome = params.attacker_outcome },
        .defender = { .id      = dwelling.id,
                      .outcome = params.defender_outcome },
    };
    return combat_effects_msg( W.ss(), combat );
  };

  SECTION( "soldier, loses" ) {
    params = {
        .attacker = e_unit_type::soldier,
        .winner   = e_combat_winner::defender,
        .attacker_outcome =
            EuroUnitCombatOutcome::demoted{
                .to = e_unit_type::free_colonist },
        .defender_outcome = DwellingCombatOutcome::no_change{},
    };
    expected = {
        .summaries = { .attacker =
                           "[Sioux] defeat [Dutch] [Soldier] "
                           "near a Sioux camp!" },
        .attacker  = {
             .for_both = { "[Dutch] [Soldier] routed! Unit "
                            "demoted to colonist status." } } };
    REQUIRE( run() == expected );
  }

  SECTION( "soldier, wins, populated decrease" ) {
    params = {
        .attacker         = e_unit_type::soldier,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome =
            DwellingCombatOutcome::population_decrease{},
    };
    expected = {
        .summaries = { .attacker =
                           "[Dutch] Soldier defeats [Sioux] "
                           "Brave near a Sioux camp!" } };
    REQUIRE( run() == expected );
  }

  SECTION( "soldier, wins, populated decrease" ) {
    params = {
        .attacker         = e_unit_type::soldier,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome =
            DwellingCombatOutcome::population_decrease{},
        .attacker_colony = e_colony::yes_and_visible };
    expected = {
        .summaries = { .attacker =
                           "[Dutch] Soldier defeats [Sioux] "
                           "Brave near attacker colony!" } };
    REQUIRE( run() == expected );
  }

  SECTION( "soldier, wins, burn" ) {
    params = {
        .attacker         = e_unit_type::soldier,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome =
            DwellingCombatOutcome::destruction{
                // Test that these don't add to the messages.
                .treasure_amount  = 1000,
                .convert_produced = true },
    };
    expected = {
        .summaries = { .attacker =
                           "[Dutch] Soldier defeats [Sioux] "
                           "Brave near a Sioux camp!" } };
    REQUIRE( run() == expected );
  }
}

TEST_CASE(
    "[combat-effects] combat_effects_msg, "
    "CombatEuroAttackUndefendedColony" ) {
  World                 W;
  CombatEffectsMessages expected;

  enum class e_colony {
    // Order matters here; needs to be in order of increasing
    // visibility.
    no,
    yes_and_visible_to_owner,
    yes_and_visible_to_both,
  };

  struct Params {
    UnitType                      attacker = {};
    e_unit_type                   defender = {};
    e_combat_winner               winner   = {};
    EuroUnitCombatOutcome         attacker_outcome;
    EuroColonyWorkerCombatOutcome defender_outcome;
    e_colony                      attacker_colony = {};
  } params;

  auto run = [&] {
    if( params.attacker_colony > e_colony::no ) {
      Colony& colony = W.add_colony( W.kAttackerColonyCoord,
                                     W.kAttackerNation );
      colony.name    = "attacker colony";
      if( params.attacker_colony >=
          e_colony::yes_and_visible_to_owner ) {
        W.map_updater().make_squares_visible(
            W.kAttackerNation, { colony.location } );
        if( params.attacker_colony >
            e_colony::yes_and_visible_to_owner ) {
          W.map_updater().make_squares_visible(
              W.kDefenderNation, { colony.location } );
        }
      }
    }
    Colony& defender_colony = W.add_colony(
        W.kDefenderColonyCoord, W.kDefenderNation );
    defender_colony.name = "defender colony";
    Unit const& defender = W.add_unit_indoors(
        defender_colony.id, e_indoor_job::bells,
        params.defender );
    W.map_updater().make_squares_visible(
        W.kDefenderNation, { defender_colony.location } );
    W.map_updater().make_squares_visible(
        W.kAttackerNation, { defender_colony.location } );
    Unit const& attacker = W.add_unit_on_map(
        params.attacker, W.kEuroColonyAttackerCoord,
        W.kAttackerNation );
    CombatEuroAttackUndefendedColony const combat{
        .winner    = params.winner,
        .colony_id = defender_colony.id,
        .attacker  = { .id      = attacker.id(),
                       .outcome = params.attacker_outcome },
        .defender  = { .id      = defender.id(),
                       .outcome = params.defender_outcome },
    };
    return combat_effects_msg( W.ss(), combat );
  };

  SECTION(
      "(artillery,free_colonist) -> "
      "(damaged_artillery,free_colonist)" ) {
    params   = { .attacker = e_unit_type::artillery,
                 .defender = e_unit_type::free_colonist,
                 .winner   = e_combat_winner::defender,
                 .attacker_outcome =
                     EuroUnitCombatOutcome::demoted{
                         .to = e_unit_type::damaged_artillery },
                 .defender_outcome =
                     EuroColonyWorkerCombatOutcome::no_change{} };
    expected = {
        .summaries = { .attacker =
                           "[French] Free Colonist defeats "
                           "[Dutch] in defender colony!",
                       .defender =
                           "[French] Free Colonist defeats "
                           "[Dutch] in defender colony!" },
        .attacker  = {
             .for_both = { "[Dutch] Artillery [damaged]. Further "
                            "damage will destroy it." } } };
    REQUIRE( run() == expected );
  }

  SECTION(
      "(soldier,free_colonist) -> (veteran soldier,captured)" ) {
    params   = { .attacker = e_unit_type::soldier,
                 .defender = e_unit_type::free_colonist,
                 .winner   = e_combat_winner::attacker,
                 .attacker_outcome =
                     EuroUnitCombatOutcome::promoted{
                         .to = e_unit_type::veteran_soldier },
                 .defender_outcome =
                     EuroColonyWorkerCombatOutcome::defeated{} };
    expected = { .attacker = {
                     .for_owner = {
                         "[Dutch] Soldier promoted to [Veteran "
                         "Soldier] for victory in combat!" } } };
    REQUIRE( run() == expected );
  }

  SECTION( "(artillery,free_colonist) -> captured" ) {
    params = {
        .attacker         = e_unit_type::artillery,
        .defender         = e_unit_type::free_colonist,
        .winner           = e_combat_winner::attacker,
        .attacker_outcome = EuroUnitCombatOutcome::no_change{},
        .defender_outcome =
            EuroColonyWorkerCombatOutcome::defeated{} };
    expected = {};
    REQUIRE( run() == expected );
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

  SECTION( "colony worker destroyed" ) {
    Colony const& colony = W.add_colony( { .x = 1, .y = 1 } );
    unit_id =
        W.add_unit_indoors( colony.id, e_indoor_job::bells )
            .id();
    outcome = EuroUnitCombatOutcome::destroyed{};
    f();
    REQUIRE( !W.units().exists( unit_id ) );
    // !! No further tests since unit does not exist.
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
    REQUIRE( tribe_type_for_unit( W.ss(), unit ) ==
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
    REQUIRE( tribe_type_for_unit( W.ss(), unit ) ==
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
    REQUIRE( tribe_type_for_unit( W.ss(), unit ) ==
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
    REQUIRE( tribe_type_for_unit( W.ss(), unit ) ==
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
    REQUIRE( tribe_type_for_unit( W.ss(), unit ) ==
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

TEST_CASE( "[combat-effects] mix_combat_effects_msgs" ) {
  CombatEffectsMessages      msgs;
  MixedCombatEffectsMessages expected;

  auto f = [&] { return mix_combat_effects_msgs( msgs ); };

  msgs.summaries = {
      .attacker = "xxx xxx",
      .defender = "yyy yyy",
  };

  msgs.attacker = { .for_owner = { "111 111", "222 222" },
                    .for_other = { "333 333", "444 444" },
                    .for_both  = { "555 555", "666 666" } };

  msgs.defender = { .for_owner = { "777 777", "888 888" },
                    .for_other = { "999 999", "aaa aaa" },
                    .for_both  = { "bbb bbb", "ccc ccc" } };

  expected = {
      .summaries = { .attacker = "xxx xxx",
                     .defender = "yyy yyy" },
      .attacker  = { "555 555", "666 666", "111 111", "222 222",
                     "bbb bbb", "ccc ccc", "999 999", "aaa aaa" },
      .defender  = { "bbb bbb", "ccc ccc", "777 777", "888 888",
                     "555 555", "666 666", "333 333", "444 444" },
  };
  REQUIRE( f() == expected );
}

TEST_CASE( "[combat-effects] filter_combat_effects_msgs" ) {
  MixedCombatEffectsMessages         msgs;
  FilteredMixedCombatEffectsMessages expected;

  auto f = [&] { return filter_combat_effects_msgs( msgs ); };

  msgs = {
      .summaries = { .attacker = "xxx xxx",
                     .defender = "yyy yyy" },
      .attacker  = { "555 555", "666 666", "111 111", "222 222",
                     "bbb bbb", "ccc ccc", "999 999", "aaa aaa" },
      .defender  = { "bbb bbb", "ccc ccc", "777 777", "888 888",
                     "555 555", "666 666", "333 333", "444 444" },
  };
  expected = {
      .attacker = { "555 555", "666 666", "111 111", "222 222",
                    "bbb bbb", "ccc ccc", "999 999", "aaa aaa" },
      .defender = { "bbb bbb", "ccc ccc", "777 777", "888 888",
                    "555 555", "666 666", "333 333", "444 444" },
  };
  REQUIRE( f() == expected );

  msgs = {
      .summaries = { .defender = "yyy yyy" },
      .attacker  = { "555 555", "666 666", "111 111", "222 222",
                     "bbb bbb", "ccc ccc", "999 999", "aaa aaa" },
  };
  expected = {
      .attacker = { "555 555", "666 666", "111 111", "222 222",
                    "bbb bbb", "ccc ccc", "999 999", "aaa aaa" },
      .defender = { "yyy yyy" },
  };
  REQUIRE( f() == expected );

  msgs     = { .summaries = { .defender = "yyy yyy" } };
  expected = { .defender = { "yyy yyy" } };
  REQUIRE( f() == expected );

  msgs     = { .summaries = { .defender = "" } };
  expected = {};
  REQUIRE( f() == expected );

  msgs     = {};
  expected = {};
  REQUIRE( f() == expected );
}

TEST_CASE( "[combat-effects] show_combat_effects_msg" ) {
  World          W;
  MockIEuroMind& attacker_mind = W.euro_mind( e_nation::dutch );
  MockIEuroMind& defender_mind = W.euro_mind( e_nation::french );

  FilteredMixedCombatEffectsMessages msgs;

  auto f = [&] {
    co_await_test( show_combat_effects_msg( msgs, attacker_mind,
                                            defender_mind ) );
  };

  msgs = {};
  f();
  attacker_mind.queue__message_box.ensure_expectations();
  defender_mind.queue__message_box.ensure_expectations();

  msgs = { .attacker = { "xxx" } };
  attacker_mind.EXPECT__message_box( "xxx" );
  f();
  attacker_mind.queue__message_box.ensure_expectations();
  defender_mind.queue__message_box.ensure_expectations();

  msgs = { .attacker = { "xxx", "yyy" },
           .defender = { "aaa", "bbb" } };
  attacker_mind.EXPECT__message_box( "xxx" );
  attacker_mind.EXPECT__message_box( "yyy" );
  defender_mind.EXPECT__message_box( "aaa" );
  defender_mind.EXPECT__message_box( "bbb" );
  f();
  attacker_mind.queue__message_box.ensure_expectations();
  defender_mind.queue__message_box.ensure_expectations();
}

} // namespace
} // namespace rn
