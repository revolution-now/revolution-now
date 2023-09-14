/****************************************************************
**enter-dwelling.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-06.
*
* Description: Unit tests for the src/enter-dwelling.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/enter-dwelling.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/ieuro-mind.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/irand.hpp"
#include "test/mocks/land-view-plane.hpp"

// Revolution Now
#include "src/imap-updater.hpp"
#include "src/mock/matchers.hpp"
#include "src/plane-stack.hpp"
#include "src/unit-mgr.hpp"
#include "src/visibility.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/tribe.rds.hpp"
#include "ss/turn.rds.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

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
  World() : Base() {
    add_player( e_nation::english );
    add_player( e_nation::french );
    set_default_player( e_nation::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _, L, L, L, L, L, L, L, L, L, L, L, L, L,
      L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, _, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, _, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
      _, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 16 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[enter-dwelling] enter_native_dwelling_options" ) {
  World     W;
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::iroquois );
  Tribe&      tribe = W.natives().tribe_for( e_tribe::iroquois );
  Unit const& missionary = W.add_free_unit(
      e_unit_type::jesuit_missionary, e_nation::english );
  Unit const& foreign_missionary = W.add_free_unit(
      e_unit_type::jesuit_missionary, e_nation::french );
  TribeRelationship& relationship =
      tribe.relationship[W.default_nation()];
  e_unit_type                unit_type = {};
  EnterNativeDwellingOptions expected;

  auto f = [&] {
    return enter_native_dwelling_options(
        W.ss(), W.default_player(), unit_type, dwelling );
  };

  dwelling.relationship[W.default_nation()].dwelling_only_alarm =
      50;
  expected.dwelling_id = dwelling.id;

  // Free colonist, no contact.
  relationship.encountered = false;
  unit_type                = e_unit_type::free_colonist;
  expected.reaction = e_enter_dwelling_reaction::wave_happily;
  expected.options  = {
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Free colonist, with contact, no war.
  relationship.encountered = true;
  relationship.at_war      = false;
  unit_type                = e_unit_type::free_colonist;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.options = {
      e_enter_dwelling_option::live_among_the_natives,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Free colonist, with contact, with war.
  unit_type           = e_unit_type::free_colonist;
  relationship.at_war = true;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.options = {
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Petty Criminal, with contact, no war.
  relationship.encountered = true;
  relationship.at_war      = false;
  unit_type                = e_unit_type::petty_criminal;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.options = {
      e_enter_dwelling_option::live_among_the_natives,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Petty Criminal, with contact, with war.
  unit_type           = e_unit_type::petty_criminal;
  relationship.at_war = true;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.options = {
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Expert Farmer, with contact, no war.
  relationship.encountered = true;
  relationship.at_war      = false;
  unit_type                = e_unit_type::expert_farmer;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.options = {
      e_enter_dwelling_option::live_among_the_natives,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Expert Farmer, with contact, with war.
  unit_type           = e_unit_type::expert_farmer;
  relationship.at_war = true;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.options = {
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Scout, with contact, no war.
  relationship.at_war = false;
  unit_type           = e_unit_type::scout;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.options = {
      e_enter_dwelling_option::speak_with_chief,
      e_enter_dwelling_option::attack_village,
      // e_enter_dwelling_option::demand_tribute,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Seasoned scout, with contact, with war.
  relationship.at_war = true;
  unit_type           = e_unit_type::seasoned_scout;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.options = {
      e_enter_dwelling_option::speak_with_chief,
      e_enter_dwelling_option::attack_village,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Military, with contact, no war.
  relationship.at_war = false;
  unit_type           = e_unit_type::dragoon;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.options = {
      e_enter_dwelling_option::attack_village,
      // e_enter_dwelling_option::demand_tribute,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Military, with contact, with war.
  relationship.at_war = true;
  unit_type           = e_unit_type::artillery;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.options = {
      e_enter_dwelling_option::attack_village,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Missionary, with contact, no war, with friendly mission.
  relationship.at_war = false;
  unit_type           = e_unit_type::missionary;
  // This unit doesn't exist but that should be ok for the pur-
  // poses of this test.
  unit_ownership_change_non_interactive(
      W.ss(), missionary.id(),
      EuroUnitOwnershipChangeTo::dwelling{ .dwelling_id =
                                               dwelling.id } );
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.options = {
      // e_enter_dwelling_option::incite_indians,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Missionary, with contact, no war, no mission.
  relationship.at_war = false;
  unit_type           = e_unit_type::missionary;
  unit_ownership_change_non_interactive(
      W.ss(), missionary.id(),
      EuroUnitOwnershipChangeTo::free{} );
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.options = {
      e_enter_dwelling_option::establish_mission,
      // e_enter_dwelling_option::incite_indians,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Missionary, with contact, no war, foreign mission.
  relationship.at_war = false;
  unit_type           = e_unit_type::missionary;
  unit_ownership_change_non_interactive(
      W.ss(), missionary.id(),
      EuroUnitOwnershipChangeTo::free{} );
  unit_ownership_change_non_interactive(
      W.ss(), foreign_missionary.id(),
      EuroUnitOwnershipChangeTo::dwelling{ .dwelling_id =
                                               dwelling.id } );
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.options = {
      // e_enter_dwelling_option::denounce_foreign_mission,
      // e_enter_dwelling_option::incite_indians,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );
  unit_ownership_change_non_interactive(
      W.ss(), foreign_missionary.id(),
      EuroUnitOwnershipChangeTo::free{} );

  // Missionary, with contact, with war, no mission.
  relationship.at_war = true;
  unit_type           = e_unit_type::missionary;
  unit_ownership_change_non_interactive(
      W.ss(), missionary.id(),
      EuroUnitOwnershipChangeTo::free{} );
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.options = {
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Trade, with contact, no war.
  relationship.at_war = false;
  unit_type           = e_unit_type::wagon_train;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.options = {
      // e_enter_dwelling_option::trade,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Trade, with contact, with war.
  relationship.at_war = true;
  unit_type           = e_unit_type::merchantman;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.options = {
      // e_enter_dwelling_option::trade,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // No category, with contact, no war.
  relationship.at_war = false;
  unit_type           = e_unit_type::treasure;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.options = {
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );
}

TEST_CASE( "[enter-dwelling] dwelling entry not encountered" ) {
  World           W;
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::iroquois );
  EnterNativeDwellingOptions options;
  e_enter_dwelling_option    expected = {};

  auto f = [&] {
    wait<e_enter_dwelling_option> w =
        present_dwelling_entry_options(
            W.ss(), W.ts(), W.default_player(), options );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    return *w;
  };

  options = {
      .dwelling_id = dwelling.id,
      .reaction    = e_enter_dwelling_reaction::wave_happily,
      .options     = {
          e_enter_dwelling_option::attack_village,
          e_enter_dwelling_option::live_among_the_natives } };

  W.gui().EXPECT__message_box( StrContains( "on land first" ) );
  expected = e_enter_dwelling_option::cancel;
  REQUIRE( f() == expected );
}

TEST_CASE( "[enter-dwelling] present_dwelling_entry_options" ) {
  World           W;
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::iroquois );
  EnterNativeDwellingOptions options;
  e_enter_dwelling_option    expected = {};
  W.iroquois().relationship[W.default_nation()].encountered =
      true;

  auto f = [&] {
    wait<e_enter_dwelling_option> w =
        present_dwelling_entry_options(
            W.ss(), W.ts(), W.default_player(), options );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    return *w;
  };

  options = {
      .dwelling_id = dwelling.id,
      .reaction    = e_enter_dwelling_reaction::wave_happily,
      .options     = {
          e_enter_dwelling_option::attack_village,
          e_enter_dwelling_option::live_among_the_natives } };

  W.euro_mind().EXPECT__show_woodcut(
      e_woodcut::entering_native_village );
  W.gui()
      .EXPECT__choice( _, e_input_required::no )
      .returns<maybe<string>>( "attack_village" );
  expected = e_enter_dwelling_option::attack_village;
  REQUIRE( f() == expected );

  // Again, no woodcut.
  W.gui()
      .EXPECT__choice( _, e_input_required::no )
      .returns<maybe<string>>( "attack_village" );
  expected = e_enter_dwelling_option::attack_village;
  REQUIRE( f() == expected );

  // Once more, with brave on top.
  W.add_native_unit_on_map( e_native_unit_type::brave,
                            { .x = 1, .y = 1 }, dwelling.id );
  W.gui()
      .EXPECT__choice( _, e_input_required::no )
      .returns<maybe<string>>( "attack_village" );
  expected = e_enter_dwelling_option::attack_brave_on_dwelling;
  REQUIRE( f() == expected );
}

TEST_CASE( "[enter-dwelling] compute_live_among_the_natives" ) {
  World     W;
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::tupi );
  dwelling.teaches = e_native_skill::ore_mining;
  Tribe& tribe     = W.natives().tribe_for( e_tribe::tupi );
  tribe.relationship[W.default_nation()].encountered = true;
  TribeRelationship& relationship =
      tribe.relationship[W.default_nation()];
  LiveAmongTheNatives expected;
  UnitComposition     comp;

  auto f = [&] {
    Unit const unit =
        create_unregistered_unit( W.default_player(), comp );
    return compute_live_among_the_natives( W.ss(), dwelling,
                                           unit );
  };

  // generally_ineligible.
  expected = LiveAmongTheNatives::generally_ineligible{};
  comp     = e_unit_type::artillery;
  REQUIRE( f() == expected );

  // petty_criminal.
  expected = LiveAmongTheNatives::petty_criminal{};
  comp     = e_unit_type::petty_criminal;
  REQUIRE( f() == expected );

  // native_convert.
  expected = LiveAmongTheNatives::native_convert{};
  comp     = e_unit_type::native_convert;
  REQUIRE( f() == expected );

  // unhappy (tribal).
  expected                  = LiveAmongTheNatives::unhappy{};
  relationship.tribal_alarm = 99;
  dwelling.relationship[W.default_nation()].dwelling_only_alarm =
      0;
  comp = e_unit_type::free_colonist;
  REQUIRE( f() == expected );

  // unhappy (dwelling).
  expected                  = LiveAmongTheNatives::unhappy{};
  relationship.tribal_alarm = 0;
  dwelling.relationship[W.default_nation()].dwelling_only_alarm =
      99;
  comp = e_unit_type::free_colonist;
  REQUIRE( f() == expected );

  relationship.tribal_alarm = 0;
  dwelling.relationship[W.default_nation()].dwelling_only_alarm =
      0;

  // already_taught.
  dwelling.has_taught = true;
  expected            = LiveAmongTheNatives::already_taught{};
  comp                = e_unit_type::free_colonist;
  REQUIRE( f() == expected );
  dwelling.has_taught = false;

  // has_expertise.
  expected = LiveAmongTheNatives::has_expertise{
      .in_what = e_unit_activity::fishing };
  comp = e_unit_type::expert_fisherman;
  REQUIRE( f() == expected );

  // has_expertise.
  expected = LiveAmongTheNatives::has_expertise{
      .in_what = e_unit_activity::pioneering };
  comp = e_unit_type::hardy_pioneer;
  REQUIRE( f() == expected );

  // promoted (servant).
  expected = LiveAmongTheNatives::promoted{
      .to = e_unit_type::expert_ore_miner };
  comp = e_unit_type::indentured_servant;
  REQUIRE( f() == expected );

  // promoted (free colonist).
  expected = LiveAmongTheNatives::promoted{
      .to = e_unit_type::expert_ore_miner };
  comp = e_unit_type::free_colonist;
  REQUIRE( f() == expected );

  // promoted (pioneer 80 tools).
  expected = LiveAmongTheNatives::promoted{
      .to = UnitComposition::create(
                UnitType::create( e_unit_type::pioneer,
                                  e_unit_type::expert_ore_miner )
                    .value(),
                { { e_unit_inventory::tools, 80 } } )
                .value() };
  comp = UnitComposition::create(
             UnitType::create( e_unit_type::pioneer,
                               e_unit_type::indentured_servant )
                 .value(),
             { { e_unit_inventory::tools, 80 } } )
             .value();
  REQUIRE( f() == expected );
}

#ifndef COMPILER_GCC
TEST_CASE( "[enter-dwelling] do_live_among_the_natives" ) {
  World             W;
  MockLandViewPlane mock_land_view;
  W.planes().back().land_view = &mock_land_view;
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
  Unit& unit = W.add_unit_on_map( e_unit_type::free_colonist,
                                  { .x = 1, .y = 1 } );
  LiveAmongTheNatives outcome;

  auto f = [&] {
    wait<> w = do_live_among_the_natives(
        W.ss(), W.ts(), dwelling, unit, outcome );
    CHECK( !w.exception() );
    CHECK( w.ready() );
  };

  // Not eligible.
  outcome = LiveAmongTheNatives::generally_ineligible{};
  W.gui()
      .EXPECT__message_box( StrContains( "not eligible" ) )
      .returns<wait<>>( make_wait<>() );
  f();
  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( dwelling.has_taught == false );

  // Eligible buy decline.
  outcome = LiveAmongTheNatives::promoted{
      .to = e_unit_type::expert_cotton_planter };
  W.gui()
      .EXPECT__choice( _, e_input_required::no )
      .returns<maybe<string>>( "no" );
  f();
  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( dwelling.has_taught == false );

  // Eligible and accept.
  outcome = LiveAmongTheNatives::promoted{
      .to = e_unit_type::expert_cotton_planter };
  W.gui()
      .EXPECT__choice( _, e_input_required::no )
      .returns<maybe<string>>( "yes" );

  W.gui()
      .EXPECT__message_box(
          Matches( "Congratulations.*Cotton.*"s ) )
      .returns<wait<>>( make_wait<>() );
  mock_land_view.EXPECT__animate( _ );
  f();
  REQUIRE( unit.type() == e_unit_type::expert_cotton_planter );
  REQUIRE( dwelling.has_taught == true );
}
#endif

TEST_CASE( "[enter-dwelling] compute_speak_with_chief" ) {
  World         W;
  Unit const*   p_unit = nullptr;
  vector<Coord> expected_tiles;
  expected_tiles.reserve( 15 * 15 ); // should be enough.
  Dwelling& dwelling_tupi =
      W.add_dwelling( { .x = 4, .y = 4 }, e_tribe::tupi );
  W.add_tribe( e_tribe::tupi )
      .relationship[W.default_nation()]
      .encountered = true;
  DwellingRelationship& relationship =
      dwelling_tupi.relationship[W.default_nation()];
  Unit& scout_petty = W.add_unit_on_map(
      UnitType::create( e_unit_type::scout,
                        e_unit_type::petty_criminal )
          .value(),
      { .x = 3, .y = 3 } );
  Unit& scout_other_expert = W.add_unit_on_map(
      UnitType::create( e_unit_type::scout,
                        e_unit_type::expert_farmer )
          .value(),
      { .x = 3, .y = 3 } );
  Unit& scout_seasoned = W.add_unit_on_map(
      e_unit_type::seasoned_scout, { .x = 3, .y = 3 } );

  // Prepare dwelling.
  dwelling_tupi.teaches = e_native_skill::cotton_planting;
  dwelling_tupi.trading.seeking_primary =
      e_commodity::trade_goods;
  dwelling_tupi.trading.seeking_secondary_1 = e_commodity::ore;
  dwelling_tupi.trading.seeking_secondary_2 =
      e_commodity::horses;

  Dwelling& dwelling_inca =
      W.add_dwelling( { .x = 5, .y = 4 }, e_tribe::inca );
  DwellingId const dwelling_inca_id = dwelling_inca.id;
  dwelling_inca                     = dwelling_tupi;
  dwelling_inca.id                  = dwelling_inca_id;
  Dwelling* dwelling                = &dwelling_tupi;

  SpeakWithChiefResult expected{
      .expertise         = e_native_skill::cotton_planting,
      .primary_trade     = e_commodity::trade_goods,
      .secondary_trade_1 = e_commodity::ore,
      .secondary_trade_2 = e_commodity::horses };

  auto f = [&] {
    CHECK( p_unit != nullptr );
    return compute_speak_with_chief( W.ss(), W.ts(), *dwelling,
                                     *p_unit );
  };

  // Target practice certain.
  p_unit                           = &scout_petty;
  relationship.dwelling_only_alarm = 99;
  W.rand().EXPECT__bernoulli( 1.0 ).returns( true );
  expected.action = ChiefAction::target_practice{};
  REQUIRE( f() == expected );

  // Target practice certain.
  p_unit                           = &scout_petty;
  relationship.dwelling_only_alarm = 80;
  W.rand().EXPECT__bernoulli( 1.0 ).returns( true );
  expected.action = ChiefAction::target_practice{};
  REQUIRE( f() == expected );

  // Target practice almost certain.
  p_unit                           = &scout_petty;
  relationship.dwelling_only_alarm = 77;
  W.rand().EXPECT__bernoulli( .95 ).returns( true );
  expected.action = ChiefAction::target_practice{};
  REQUIRE( f() == expected );

  // Target practice sometimes
  p_unit                           = &scout_petty;
  relationship.dwelling_only_alarm = 50;
  W.rand().EXPECT__bernoulli( .50 ).returns( true );
  expected.action = ChiefAction::target_practice{};
  REQUIRE( f() == expected );

  // Target practice sometimes
  p_unit                           = &scout_seasoned;
  relationship.dwelling_only_alarm = 55;
  W.rand().EXPECT__bernoulli( .50 ).returns( true );
  expected.action = ChiefAction::target_practice{};
  REQUIRE( f() == expected );

  // Target practice almost never.
  p_unit                           = &scout_petty;
  relationship.dwelling_only_alarm = 23;
  W.rand().EXPECT__bernoulli( .05 ).returns( true );
  expected.action = ChiefAction::target_practice{};
  REQUIRE( f() == expected );

  relationship.has_spoken_with_chief = true;

  // Target practice never.
  p_unit                           = &scout_petty;
  relationship.dwelling_only_alarm = 20;
  W.rand().EXPECT__bernoulli( 0.0 ).returns( false );
  expected.action = ChiefAction::none{};
  REQUIRE( f() == expected );

  // Target practice never.
  p_unit                           = &scout_petty;
  relationship.dwelling_only_alarm = 0;
  W.rand().EXPECT__bernoulli( 0.0 ).returns( false );
  expected.action = ChiefAction::none{};
  REQUIRE( f() == expected );

  relationship.has_spoken_with_chief = false;
  relationship.dwelling_only_alarm   = 0;

  // outcome: none.
  p_unit = &scout_petty;
  W.rand().EXPECT__bernoulli( 0.0 ).returns( false );
  W.rand()
      .EXPECT__between_ints( 0, 100, e_interval::half_open )
      .returns( 0 );
  expected.action = ChiefAction::none{};
  REQUIRE( f() == expected );

  // outcome: gift.
  p_unit = &scout_petty;
  W.rand().EXPECT__bernoulli( 0.0 ).returns( false );
  W.rand()
      .EXPECT__between_ints( 0, 100, e_interval::half_open )
      .returns( 33 );
  W.rand()
      .EXPECT__between_ints( 50, 300, e_interval::closed )
      .returns( 111 );
  expected.action = ChiefAction::gift_money{ .quantity = 111 };
  REQUIRE( f() == expected );

  // outcome: gift + seasoned + civilized.
  W.add_tribe( e_tribe::inca )
      .relationship[W.default_nation()]
      .encountered = true;
  dwelling         = &dwelling_inca;
  p_unit           = &scout_seasoned;
  W.rand().EXPECT__bernoulli( 0.0 ).returns( false );
  W.rand()
      .EXPECT__between_ints( 0, 100, e_interval::half_open )
      .returns( 20 );
  W.rand()
      .EXPECT__between_ints( 166, 2000, e_interval::closed )
      .returns( 1111 );
  expected.action = ChiefAction::gift_money{ .quantity = 1111 };
  REQUIRE( f() == expected );
  dwelling = &dwelling_tupi;

  // outcome: promotion.
  p_unit = &scout_petty;
  W.rand().EXPECT__bernoulli( 0.0 ).returns( false );
  W.rand()
      .EXPECT__between_ints( 0, 100, e_interval::half_open )
      .returns( 80 );
  expected.action = ChiefAction::promotion{};
  REQUIRE( f() == expected );

  // outcome: failed promotion.
  p_unit = &scout_other_expert;
  W.rand().EXPECT__bernoulli( 0.0 ).returns( false );
  W.rand()
      .EXPECT__between_ints( 0, 100, e_interval::half_open )
      .returns( 80 );
  expected.action = ChiefAction::none{};
  REQUIRE( f() == expected );

  // outcome: tales of nearby land non-seasoned.
  p_unit = &scout_petty;
  W.rand().EXPECT__bernoulli( 0.0 ).returns( false );
  W.rand()
      .EXPECT__between_ints( 0, 100, e_interval::half_open )
      .returns( 73 );
  expected_tiles.clear();
  for( int y = 4 - 9 / 2; y < 4 + 1 + 9 / 2; ++y ) {
    for( int x = 4 - 9 / 2; x < 4 + 1 + 9 / 2; ++x ) {
      Coord const c{ .x = x, .y = y };
      if( W.square( c ).surface == e_surface::water ) continue;
      // Remove squares in the radius of the scout, which are al-
      // ready visible.
      static Rect const scout_visible{
          .x = 1, .y = 1, .w = 5, .h = 5 };
      if( c.is_inside( scout_visible ) ) continue;
      expected_tiles.push_back( c );
    }
  }
  REQUIRE_FALSE( expected_tiles.empty() );
  expected.action = ChiefAction::tales_of_nearby_lands{
      .tiles = expected_tiles };
  REQUIRE( f() == expected );

  // outcome: tales of nearby land seasoned.
  p_unit = &scout_seasoned;
  W.rand().EXPECT__bernoulli( 0.0 ).returns( false );
  W.rand()
      .EXPECT__between_ints( 0, 100, e_interval::half_open )
      .returns( 70 );
  expected_tiles.clear();
  for( int y = 4 - 13 / 2; y < 4 + 1 + 13 / 2; ++y ) {
    for( int x = 4 - 13 / 2; x < 4 + 1 + 13 / 2; ++x ) {
      Coord const c{ .x = x, .y = y };
      if( x < 0 || y < 0 ) continue;
      if( W.square( c ).surface == e_surface::water ) continue;
      // Remove squares in the radius of the scout, which are al-
      // ready visible.
      static Rect const scout_visible{
          .x = 1, .y = 1, .w = 5, .h = 5 };
      if( c.is_inside( scout_visible ) ) continue;
      expected_tiles.push_back( c );
    }
  }
  REQUIRE_FALSE( expected_tiles.empty() );
  expected.action = ChiefAction::tales_of_nearby_lands{
      .tiles = expected_tiles };
  REQUIRE( f() == expected );
}

TEST_CASE( "[enter-dwelling] do_speak_with_chief" ) {
  World             W;
  MockLandViewPlane mock_land_view;
  W.planes().back().land_view = &mock_land_view;
  Player const&        player = W.default_player();
  SpeakWithChiefResult outcome;
  Unit*                p_unit = nullptr;
  Dwelling&            dwelling =
      W.add_dwelling( { .x = 4, .y = 4 }, e_tribe::tupi );
  W.add_tribe( e_tribe::tupi )
      .relationship[W.default_nation()]
      .encountered = true;
  DwellingRelationship& relationship =
      dwelling.relationship[W.default_nation()];
  Unit& scout_petty = W.add_unit_on_map(
      UnitType::create( e_unit_type::scout,
                        e_unit_type::petty_criminal )
          .value(),
      { .x = 3, .y = 3 } );
  REQUIRE_FALSE( relationship.has_spoken_with_chief );

  outcome = { .expertise     = e_native_skill::tobacco_planting,
              .primary_trade = e_commodity::horses,
              .secondary_trade_1 = e_commodity::muskets,
              .secondary_trade_2 = e_commodity::cloth };

  auto f = [&] {
    wait<> w = do_speak_with_chief( W.ss(), W.ts(), dwelling,
                                    W.default_player(), *p_unit,
                                    outcome );
    REQUIRE( !w.has_exception() );
    REQUIRE( w.ready() );
  };

  SECTION( "none" ) {
    p_unit         = &scout_petty;
    outcome.action = ChiefAction::none{};

    W.gui().EXPECT__message_box(
        StrContains( "Greetings traveler" ) );

    W.gui().EXPECT__message_box(
        StrContains( "We always welcome" ) );
    f();
    REQUIRE( player.money == 0 );
    REQUIRE( p_unit->type() == scout_petty.type() );
  }

  SECTION( "gift_money" ) {
    p_unit         = &scout_petty;
    outcome.action = ChiefAction::gift_money{ .quantity = 111 };

    W.gui().EXPECT__message_box(
        StrContains( "Greetings traveler" ) );

    W.gui().EXPECT__message_box(
        StrContains( "Please take these" ) );
    f();
    REQUIRE( player.money == 111 );
    REQUIRE( p_unit->type() == scout_petty.type() );
  }

  SECTION( "tales" ) {
    using namespace std::literals::chrono_literals;
    p_unit         = &scout_petty;
    outcome.action = ChiefAction::tales_of_nearby_lands{
        .tiles = { { .x = 0, .y = 6 }, { .x = 1, .y = 6 } } };

    W.gui().EXPECT__message_box(
        StrContains( "Greetings traveler" ) );
    vector<int> const shuffled_indices{ 1, 0 };
    expect_shuffle( W.rand(), shuffled_indices );

    W.gui().EXPECT__message_box(
        StrContains( "sit around the campfire" ) );
    mock_land_view.EXPECT__center_on_tile(
        Coord{ .x = 4, .y = 4 } );
    W.gui().EXPECT__wait_for( 20ms ).returns( 20000us );
    W.gui().EXPECT__wait_for( 20ms ).returns( 20000us );
    W.gui().EXPECT__wait_for( 600ms ).returns( 600000us );
    Visibility const viz( W.ss(), W.default_nation() );
    W.map_updater().make_squares_visible(
        W.default_nation(), { { .x = 3, .y = 6 } } );
    W.map_updater().make_squares_visible(
        W.default_nation(), { { .x = 1, .y = 6 } } );
    W.map_updater().make_squares_fogged(
        W.default_nation(), { { .x = 3, .y = 6 } } );
    W.map_updater().make_squares_fogged(
        W.default_nation(), { { .x = 1, .y = 6 } } );
    REQUIRE( viz.visible( { .x = 0, .y = 6 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 1, .y = 6 } ) ==
             e_tile_visibility::visible_with_fog );
    REQUIRE( viz.visible( { .x = 2, .y = 6 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 3, .y = 6 } ) ==
             e_tile_visibility::visible_with_fog );
    f();
    REQUIRE( viz.visible( { .x = 0, .y = 6 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 1, .y = 6 } ) ==
             e_tile_visibility::visible_and_clear );
    REQUIRE( viz.visible( { .x = 2, .y = 6 } ) ==
             e_tile_visibility::hidden );
    REQUIRE( viz.visible( { .x = 3, .y = 6 } ) ==
             e_tile_visibility::visible_with_fog );
    REQUIRE( player.money == 0 );
    REQUIRE( p_unit->type() == scout_petty.type() );
  }

  SECTION( "promotion" ) {
    p_unit         = &scout_petty;
    outcome.action = ChiefAction::promotion{};

    W.gui().EXPECT__message_box(
        StrContains( "Greetings traveler" ) );
    W.gui().EXPECT__message_box( StrContains( "send guides" ) );
    mock_land_view.EXPECT__animate( _ );
    W.gui().EXPECT__message_box(
        StrContains( "promoted to [Seasoned Scout]" ) );
    f();
    REQUIRE( player.money == 0 );
    REQUIRE( p_unit->type() == e_unit_type::seasoned_scout );
  }

  SECTION( "target_practice" ) {
    p_unit         = &scout_petty;
    outcome.action = ChiefAction::target_practice{};

    W.gui().EXPECT__message_box(
        StrContains( "violated sacred taboos" ) );
    mock_land_view.EXPECT__animate( _ );
    REQUIRE( W.units().exists( UnitId{ 1 } ) );
    f();
    REQUIRE_FALSE( W.units().exists( UnitId{ 1 } ) );
    REQUIRE( player.money == 0 );
  }

  REQUIRE( relationship.has_spoken_with_chief );
}

TEST_CASE( "[enter-dwelling] compute_establish_mission" ) {
  World                  W;
  EstablishMissionResult expected;
  Tribe&                 tribe = W.add_tribe( e_tribe::sioux );
  TribeRelationship&     relationship =
      tribe.relationship[W.default_nation()];
  relationship.encountered = true;
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::sioux );

  auto f = [&] {
    return compute_establish_mission( W.ss(), W.default_player(),
                                      dwelling );
  };

  relationship.tribal_alarm = 0;
  expected = { .reaction = e_missionary_reaction::curiosity };
  REQUIRE( f() == expected );

  relationship.tribal_alarm = 25;
  expected = { .reaction = e_missionary_reaction::cautious };
  REQUIRE( f() == expected );

  relationship.tribal_alarm = 50;
  expected = { .reaction = e_missionary_reaction::offended };
  REQUIRE( f() == expected );

  relationship.tribal_alarm = 75;
  expected = { .reaction = e_missionary_reaction::hostility };
  REQUIRE( f() == expected );
}

TEST_CASE( "[enter-dwelling] do_establish_mission" ) {
  World                  W;
  EstablishMissionResult outcome;
  Dwelling&              dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
  Unit& missionary = W.add_unit_on_map( e_unit_type::missionary,
                                        { .x = 1, .y = 0 } );
  W.turn().time_point.year = 1501;
  string msg;

  auto f = [&] {
    wait<> const w =
        do_establish_mission( W.ss(), W.ts(), W.default_player(),
                              dwelling, missionary, outcome );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
  };

  outcome = { .reaction = e_missionary_reaction::curiosity };
  msg =
      "[English] mission established in [Inca] city "
      "in the year 1501. The Inca react with [curiosity].";
  W.gui().EXPECT__message_box( msg );
  f();

  outcome = { .reaction = e_missionary_reaction::hostility };
  msg =
      "[English] mission established in [Inca] city "
      "in the year 1501. The Inca react with [hostility].";
  W.gui().EXPECT__message_box( msg );
  f();

  REQUIRE(
      as_const( W.units() ).ownership_of( missionary.id() ) ==
      UnitOwnership{
          UnitOwnership::dwelling{ .id = dwelling.id } } );
}

} // namespace
} // namespace rn
