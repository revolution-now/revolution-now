/****************************************************************
**rpt.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-20.
*
* Description: Unit tests for the src/test/rpt.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rpt.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"

// ss
#include "ss/player.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::AllOf;
using ::mock::matchers::Any;
using ::mock::matchers::Field;
using ::mock::matchers::IterableElementsAre;
using ::mock::matchers::Matches;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() { add_player( e_nation::dutch ); }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rpt] click_purchase" ) {
  World   W;
  Player& player             = W.default_player();
  player.money               = 2000;
  player.artillery_purchases = 2;

  using C             = ChoiceConfig;
  using CO            = ChoiceConfigOption;
  auto config_matcher = AllOf(
      Field( &C::msg, "What would you like to purchase?"s ),
      Field(
          &C::options,
          IterableElementsAre(
              AllOf( Field( &CO::key, "none"s ),
                     Field( &CO::display_name, "None"s ),
                     Field( &CO::disabled, false ) ),
              AllOf(
                  Field( &CO::key, "artillery"s ),
                  Field(
                      &CO::display_name,
                      Matches( R"(Artillery +\(Cost: 700\))" ) ),
                  Field( &CO::disabled, false ) ),
              AllOf( Field( &CO::key, "caravel"s ),
                     Field( &CO::display_name,
                            Matches(
                                R"(Caravel +\(Cost: 1000\))" ) ),
                     Field( &CO::disabled, false ) ),
              AllOf(
                  Field( &CO::key, "merchantman"s ),
                  Field(
                      &CO::display_name,
                      Matches(
                          R"(Merchantman +\(Cost: 2000\))" ) ),
                  Field( &CO::disabled, false ) ),
              AllOf( Field( &CO::key, "galleon"s ),
                     Field( &CO::display_name,
                            Matches(
                                R"(Galleon +\(Cost: 3000\))" ) ),
                     Field( &CO::disabled, true ) ),
              AllOf(
                  Field( &CO::key, "privateer"s ),
                  Field( &CO::display_name,
                         Matches(
                             R"(Privateer +\(Cost: 2000\))" ) ),
                  Field( &CO::disabled, false ) ),
              AllOf( Field( &CO::key, "frigate"s ),
                     Field( &CO::display_name,
                            Matches(
                                R"(Frigate +\(Cost: 5000\))" ) ),
                     Field( &CO::disabled, true ) ) ) ),
      Field( &C::initial_selection, 0 ) );

  EXPECT_CALL( W.gui(),
               choice( config_matcher, e_input_required::no ) )
      .returns( make_wait<maybe<string>>( "artillery" ) );

  UnitsState const& units = W.ss().units;
  REQUIRE( units.all().size() == 0 );
  REQUIRE( player.artillery_purchases == 2 );

  wait<> w = click_purchase( W.ss(), W.ts(), player );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );

  // Check that we've created the artillery.
  REQUIRE( player.artillery_purchases == 3 );
  REQUIRE( player.money == 1300 );
  REQUIRE( units.all().size() == 1 );
  REQUIRE( units.unit_for( UnitId{ 1 } ).type() ==
           e_unit_type::artillery );
  REQUIRE( units.unit_for( UnitId{ 1 } ).nation() ==
           player.nation );
  REQUIRE( units.ownership_of( UnitId{ 1 } )
               .holds<UnitOwnership::harbor>() );
}

TEST_CASE( "[rpt] click_train" ) {
  World   W;
  Player& player             = W.default_player();
  player.money               = 1010;
  player.artillery_purchases = 2;

  using C             = ChoiceConfig;
  using CO            = ChoiceConfigOption;
  auto config_matcher = AllOf(
      Field(
          &C::msg,
          Matches(
              ".*Royal University.*specialists.*grease.*"s ) ),
      Field(
          &C::options,
          IterableElementsAre(
              AllOf( Field( &CO::key, "none"s ),
                     Field( &CO::display_name, "None"s ),
                     Field( &CO::disabled, false ) ),
              AllOf(
                  Field( &CO::key, "expert_ore_miner"s ),
                  Field(
                      &CO::display_name,
                      Matches(
                          R"(Expert Ore Miners +\(Cost: 600\))" ) ),
                  Field( &CO::disabled, false ) ),
              Any(), // expert_lumberjack
              Any(), // master_gunsmith
              Any(), // expert_silver_miner
              Any(), // master_fur_trader
              AllOf(
                  Field( &CO::key, "expert_fisherman"s ),
                  Field(
                      &CO::display_name,
                      Matches(
                          R"(Expert Fishermen +\(Cost: 1000\))" ) ),
                  Field( &CO::disabled, false ) ),
              Any(), // master_carpenter
              AllOf(
                  Field( &CO::key, "master_blacksmith"s ),
                  Field(
                      &CO::display_name,
                      Matches(
                          R"(Master Blacksmiths +\(Cost: 1050\))" ) ),
                  Field( &CO::disabled, true ) ),
              Any(), // expert_farmer
              Any(), // master_distiller
              Any(), // hardy_pioneer
              Any(), // master_tobacconist
              Any(), // master_weaver
              Any(), // jesuit_missionary
              Any(), // firebrand_preacher
              AllOf(
                  Field( &CO::key, "elder_statesman"s ),
                  Field(
                      &CO::display_name,
                      Matches(
                          R"(Elder Statesmen +\(Cost: 1900\))" ) ),
                  Field( &CO::disabled, true ) ),
              AllOf(
                  Field( &CO::key, "veteran_soldier"s ),
                  Field(
                      &CO::display_name,
                      Matches(
                          R"(Veteran Soldiers +\(Cost: 2000\))" ) ),
                  Field( &CO::disabled, true ) ) ) ),
      Field( &C::initial_selection, 0 ) );

  EXPECT_CALL( W.gui(),
               choice( config_matcher, e_input_required::no ) )
      .returns( make_wait<maybe<string>>( "expert_fisherman" ) );

  UnitsState const& units = W.ss().units;
  REQUIRE( units.all().size() == 0 );

  wait<> w = click_train( W.ss(), W.ts(), player );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );

  // Check that we've created the fisherman.
  REQUIRE( player.money == 10 );
  REQUIRE( units.all().size() == 1 );
  REQUIRE( units.unit_for( UnitId{ 1 } ).type() ==
           e_unit_type::expert_fisherman );
  REQUIRE( units.unit_for( UnitId{ 1 } ).nation() ==
           player.nation );
  REQUIRE( units.ownership_of( UnitId{ 1 } )
               .holds<UnitOwnership::harbor>() );
}

} // namespace
} // namespace rn
