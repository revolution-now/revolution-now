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
#include "test/mocks/igui.hpp"
#include "test/mocks/land-view-plane.hpp"

// Revolution Now
#include "src/mock/matchers.hpp"
#include "src/plane-stack.hpp"
#include "src/ustate.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/tribe.rds.hpp"
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
    set_default_player( e_nation::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[enter-dwelling] enter_native_dwelling_options" ) {
  World     W;
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::iroquois );
  Tribe& tribe = W.natives().tribe_for( e_tribe::iroquois );
  maybe<TribeRelationship>& relationship =
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
  relationship      = nothing;
  unit_type         = e_unit_type::free_colonist;
  expected.reaction = e_enter_dwelling_reaction::wave_happily;
  expected.category = e_dwelling_interaction_category::colonist;
  expected.options  = {
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Free colonist, with contact, no war.
  relationship.emplace();
  relationship->at_war = false;
  unit_type            = e_unit_type::free_colonist;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.category = e_dwelling_interaction_category::colonist;
  expected.options  = {
      e_enter_dwelling_option::live_among_the_natives,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Free colonist, with contact, with war.
  unit_type            = e_unit_type::free_colonist;
  relationship->at_war = true;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.category = e_dwelling_interaction_category::colonist;
  expected.options  = {
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Petty Criminal, with contact, no war.
  relationship.emplace();
  relationship->at_war = false;
  unit_type            = e_unit_type::petty_criminal;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.category = e_dwelling_interaction_category::colonist;
  expected.options  = {
      e_enter_dwelling_option::live_among_the_natives,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Petty Criminal, with contact, with war.
  unit_type            = e_unit_type::petty_criminal;
  relationship->at_war = true;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.category = e_dwelling_interaction_category::colonist;
  expected.options  = {
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Expert Farmer, with contact, no war.
  relationship.emplace();
  relationship->at_war = false;
  unit_type            = e_unit_type::expert_farmer;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.category = e_dwelling_interaction_category::colonist;
  expected.options  = {
      e_enter_dwelling_option::live_among_the_natives,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Expert Farmer, with contact, with war.
  unit_type            = e_unit_type::expert_farmer;
  relationship->at_war = true;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.category = e_dwelling_interaction_category::colonist;
  expected.options  = {
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Scout, with contact, no war.
  relationship->at_war = false;
  unit_type            = e_unit_type::scout;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.category = e_dwelling_interaction_category::scout;
  expected.options  = {
      e_enter_dwelling_option::speak_with_chief,
      e_enter_dwelling_option::attack_village,
      e_enter_dwelling_option::demand_tribute,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Seasoned scout, with contact, with war.
  relationship->at_war = true;
  unit_type            = e_unit_type::seasoned_scout;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.category = e_dwelling_interaction_category::scout;
  expected.options  = {
      e_enter_dwelling_option::speak_with_chief,
      e_enter_dwelling_option::attack_village,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Military, with contact, no war.
  relationship->at_war = false;
  unit_type            = e_unit_type::dragoon;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.category = e_dwelling_interaction_category::military;
  expected.options  = {
      e_enter_dwelling_option::attack_village,
      e_enter_dwelling_option::demand_tribute,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Military, with contact, with war.
  relationship->at_war = true;
  unit_type            = e_unit_type::artillery;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.category = e_dwelling_interaction_category::military;
  expected.options  = {
      e_enter_dwelling_option::attack_village,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Missionary, with contact, no war, with mission.
  relationship->at_war = false;
  unit_type            = e_unit_type::missionary;
  // This unit doesn't exist but that should be ok for the pur-
  // poses of this test.
  dwelling.mission = UnitId{ 111 };
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.category =
      e_dwelling_interaction_category::missionary;
  expected.options = {
      e_enter_dwelling_option::incite_indians,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Missionary, with contact, no war, no mission.
  relationship->at_war = false;
  unit_type            = e_unit_type::missionary;
  dwelling.mission     = nothing;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.category =
      e_dwelling_interaction_category::missionary;
  expected.options = {
      e_enter_dwelling_option::establish_mission,
      e_enter_dwelling_option::incite_indians,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Missionary, with contact, with war, no mission.
  relationship->at_war = true;
  unit_type            = e_unit_type::missionary;
  dwelling.mission     = nothing;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.category =
      e_dwelling_interaction_category::missionary;
  expected.options = {
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Trade, with contact, no war.
  relationship->at_war = false;
  unit_type            = e_unit_type::wagon_train;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.category = e_dwelling_interaction_category::trade;
  expected.options  = {
      e_enter_dwelling_option::trade,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // Trade, with contact, with war.
  relationship->at_war = true;
  unit_type            = e_unit_type::merchantman;
  expected.reaction =
      e_enter_dwelling_reaction::scalps_and_war_drums;
  expected.category = e_dwelling_interaction_category::trade;
  expected.options  = {
      e_enter_dwelling_option::trade,
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );

  // No category, with contact, no war.
  relationship->at_war = false;
  unit_type            = e_unit_type::treasure;
  expected.reaction =
      e_enter_dwelling_reaction::frowning_archers;
  expected.category = e_dwelling_interaction_category::none;
  expected.options  = {
      e_enter_dwelling_option::cancel,
  };
  REQUIRE( f() == expected );
}

TEST_CASE( "[enter-dwelling] compute_live_among_the_natives" ) {
  World     W;
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::tupi );
  dwelling.teaches = e_native_skill::ore_mining;
  Tribe& tribe     = W.natives().tribe_for( e_tribe::tupi );
  tribe.relationship[W.default_nation()].emplace();
  TribeRelationship& relationship =
      *tribe.relationship[W.default_nation()];
  LiveAmongTheNatives_t expected;
  UnitComposition       comp;

  auto f = [&] {
    Unit const unit =
        create_unregistered_unit( W.default_player(), comp );
    return compute_live_among_the_natives( W.ss(), relationship,
                                           dwelling, unit );
  };

  // generally_ineligible.
  expected = LiveAmongTheNatives::generally_ineligible{};
  comp     = UnitComposition::create( e_unit_type::artillery );
  REQUIRE( f() == expected );

  // petty_criminal.
  expected = LiveAmongTheNatives::petty_criminal{};
  comp = UnitComposition::create( e_unit_type::petty_criminal );
  REQUIRE( f() == expected );

  // native_convert.
  expected = LiveAmongTheNatives::native_convert{};
  comp = UnitComposition::create( e_unit_type::native_convert );
  REQUIRE( f() == expected );

  // unhappy (tribal).
  expected                  = LiveAmongTheNatives::unhappy{};
  relationship.tribal_alarm = 99;
  dwelling.relationship[W.default_nation()].dwelling_only_alarm =
      0;
  comp = UnitComposition::create( e_unit_type::free_colonist );
  REQUIRE( f() == expected );

  // unhappy (dwelling).
  expected                  = LiveAmongTheNatives::unhappy{};
  relationship.tribal_alarm = 0;
  dwelling.relationship[W.default_nation()].dwelling_only_alarm =
      99;
  comp = UnitComposition::create( e_unit_type::free_colonist );
  REQUIRE( f() == expected );

  relationship.tribal_alarm = 0;
  dwelling.relationship[W.default_nation()].dwelling_only_alarm =
      0;

  // already_taught.
  dwelling.has_taught = true;
  expected            = LiveAmongTheNatives::already_taught{};
  comp = UnitComposition::create( e_unit_type::free_colonist );
  REQUIRE( f() == expected );
  dwelling.has_taught = false;

  // has_expertise.
  expected = LiveAmongTheNatives::has_expertise{
      .in_what = e_unit_activity::fishing };
  comp =
      UnitComposition::create( e_unit_type::expert_fisherman );
  REQUIRE( f() == expected );

  // has_expertise.
  expected = LiveAmongTheNatives::has_expertise{
      .in_what = e_unit_activity::pioneering };
  comp = UnitComposition::create( e_unit_type::hardy_pioneer );
  REQUIRE( f() == expected );

  // promoted (servant).
  expected = LiveAmongTheNatives::promoted{
      .to = UnitComposition::create(
          e_unit_type::expert_ore_miner ) };
  comp =
      UnitComposition::create( e_unit_type::indentured_servant );
  REQUIRE( f() == expected );

  // promoted (free colonist).
  expected = LiveAmongTheNatives::promoted{
      .to = UnitComposition::create(
          e_unit_type::expert_ore_miner ) };
  comp = UnitComposition::create( e_unit_type::free_colonist );
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
  LiveAmongTheNatives_t outcome;

  auto f = [&] {
    wait<> w = do_live_among_the_natives(
        W.planes(), W.ts(), dwelling, W.default_player(), unit,
        outcome );
    CHECK( !w.exception() );
    CHECK( w.ready() );
  };

  // Not eligible.
  outcome = LiveAmongTheNatives::generally_ineligible{};
  EXPECT_CALL( W.gui(),
               message_box( StrContains( "not eligible" ) ) )
      .returns<wait<>>( make_wait<>() );
  f();
  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( dwelling.has_taught == false );

  // Eligible buy decline.
  outcome = LiveAmongTheNatives::promoted{
      .to = UnitComposition::create(
          e_unit_type::expert_cotton_planter ) };
  EXPECT_CALL( W.gui(), choice( _, e_input_required::no ) )
      .returns<maybe<string>>( "no" );
  f();
  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( dwelling.has_taught == false );

  // Eligible and accept.
  outcome = LiveAmongTheNatives::promoted{
      .to = UnitComposition::create(
          e_unit_type::expert_cotton_planter ) };
  EXPECT_CALL( W.gui(), choice( _, e_input_required::no ) )
      .returns<maybe<string>>( "yes" );
  EXPECT_CALL(
      W.gui(),
      message_box( Matches( "Congratulations.*Cotton.*"s ) ) )
      .returns<wait<>>( make_wait<>() );
  EXPECT_CALL(
      mock_land_view,
      animate_unit_depixelation(
          unit.id(), e_unit_type::expert_cotton_planter ) )
      .returns<monostate>();
  f();
  REQUIRE( unit.type() == e_unit_type::expert_cotton_planter );
  REQUIRE( dwelling.has_taught == true );
}
#endif

TEST_CASE( "[enter-dwelling] compute_speak_with_chief" ) {
  World W;
  // TODO
}

TEST_CASE( "[enter-dwelling] do_speak_with_chief" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
