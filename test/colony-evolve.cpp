/****************************************************************
**colony-evolve.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-06.
*
* Description: Unit tests for the src/colony-evolve.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/colony-evolve.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/irand.hpp"

// ss
#include "src/ss/colony.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/units.hpp"

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
    add_default_player();
    create_default_map();
  }

  Coord const kColonySquare = Coord{ .x = 1, .y = 1 };

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

  void create_arctic_map() {
    MapSquare const A = make_terrain( e_terrain::arctic );
    // clang-format off
    vector<MapSquare> tiles{
      A, A, A,
      A, A, A,
      A, A, A,
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[colony-evolve] spoilage" ) {
  World   W;
  Colony& colony = W.add_colony( { .x = 1, .y = 1 } );
  for( e_commodity c : refl::enum_values<e_commodity> )
    colony.commodities[c] = 101;
  colony.commodities[e_commodity::food] = 150;
  ColonyEvolution ev                    = evolve_colony_one_turn(
      W.ss(), W.ts(), W.default_player(), colony );
  REQUIRE( ev.production.center_food_production == 5 );
  // Food is only 152 despite the 5 food produced in the center
  // square because of horse production, which in this case al-
  // lows a max of 5 horses (101+24)/25=5 to be bread, but is
  // then further constrained by the fact that it can only con-
  // sume 3 food (ceil(5/2)). That said, the increase in horses
  // won't be reflected in any of the numbers below because the
  // new horses will be suppressed because the number of horses
  // is already over capacity (so the number of horses that will
  // be reported as having spoiled is actually only 1).
  REQUIRE( colony.commodities[e_commodity::food] == 152 );
  REQUIRE( colony.commodities[e_commodity::sugar] == 100 );
  REQUIRE( colony.commodities[e_commodity::tobacco] == 100 );
  REQUIRE( colony.commodities[e_commodity::cotton] == 100 );
  REQUIRE( colony.commodities[e_commodity::fur] == 100 );
  REQUIRE( colony.commodities[e_commodity::lumber] == 100 );
  REQUIRE( colony.commodities[e_commodity::ore] == 100 );
  REQUIRE( colony.commodities[e_commodity::silver] == 100 );
  REQUIRE( colony.commodities[e_commodity::horses] == 100 );
  REQUIRE( colony.commodities[e_commodity::rum] == 100 );
  REQUIRE( colony.commodities[e_commodity::cigars] == 100 );
  REQUIRE( colony.commodities[e_commodity::cloth] == 100 );
  REQUIRE( colony.commodities[e_commodity::coats] == 100 );
  REQUIRE( colony.commodities[e_commodity::trade_goods] == 100 );
  REQUIRE( colony.commodities[e_commodity::tools] == 100 );
  REQUIRE( colony.commodities[e_commodity::muskets] == 100 );

  // In practice, for a new empty colony, there should not be any
  // other notifications.
  vector<ColonyNotification_t> expected{
      ColonyNotification::spoilage{
          .spoiled = {
              { .type = e_commodity::sugar, .quantity = 1 },
              { .type = e_commodity::tobacco, .quantity = 1 },
              { .type = e_commodity::cotton, .quantity = 1 },
              { .type = e_commodity::fur, .quantity = 1 },
              { .type = e_commodity::lumber, .quantity = 1 },
              { .type = e_commodity::ore, .quantity = 1 },
              { .type = e_commodity::silver, .quantity = 1 },
              { .type = e_commodity::horses, .quantity = 1 },
              { .type = e_commodity::rum, .quantity = 1 },
              { .type = e_commodity::cigars, .quantity = 1 },
              { .type = e_commodity::cloth, .quantity = 1 },
              { .type = e_commodity::coats, .quantity = 1 },
              { .type     = e_commodity::trade_goods,
                .quantity = 1 },
              { .type = e_commodity::tools, .quantity = 1 },
              { .type = e_commodity::muskets, .quantity = 1 },
          } } };

  REQUIRE( ev.notifications == expected );

  // Now repeat but with a warehouse.
  colony.buildings[e_colony_building::warehouse] = true;

  for( e_commodity c : refl::enum_values<e_commodity> )
    colony.commodities[c] = 201;
  colony.commodities[e_commodity::food] = 800;
  ev = evolve_colony_one_turn( W.ss(), W.ts(),
                               W.default_player(), colony );
  REQUIRE( ev.production.center_food_production == 5 );
  // Food goes up by 5 due to center square production and down
  // by 3 due to horse breeding, then down 200 due to new
  // colonist being produced, putting the final value at 602. See
  // corresponding comment above for more info on the horse
  // breeding.
  REQUIRE( colony.commodities[e_commodity::food] == 602 );
  REQUIRE( colony.commodities[e_commodity::sugar] == 200 );
  REQUIRE( colony.commodities[e_commodity::tobacco] == 200 );
  REQUIRE( colony.commodities[e_commodity::cotton] == 200 );
  REQUIRE( colony.commodities[e_commodity::fur] == 200 );
  REQUIRE( colony.commodities[e_commodity::lumber] == 200 );
  REQUIRE( colony.commodities[e_commodity::ore] == 200 );
  REQUIRE( colony.commodities[e_commodity::silver] == 200 );
  REQUIRE( colony.commodities[e_commodity::horses] == 200 );
  REQUIRE( colony.commodities[e_commodity::rum] == 200 );
  REQUIRE( colony.commodities[e_commodity::cigars] == 200 );
  REQUIRE( colony.commodities[e_commodity::cloth] == 200 );
  REQUIRE( colony.commodities[e_commodity::coats] == 200 );
  REQUIRE( colony.commodities[e_commodity::trade_goods] == 200 );
  REQUIRE( colony.commodities[e_commodity::tools] == 200 );
  REQUIRE( colony.commodities[e_commodity::muskets] == 200 );

  // In practice, for a new empty colony, there should not be any
  // other notifications.
  expected = {
      ColonyNotification::new_colonist{ .id = 1 },
      ColonyNotification::spoilage{
          .spoiled = {
              { .type = e_commodity::sugar, .quantity = 1 },
              { .type = e_commodity::tobacco, .quantity = 1 },
              { .type = e_commodity::cotton, .quantity = 1 },
              { .type = e_commodity::fur, .quantity = 1 },
              { .type = e_commodity::lumber, .quantity = 1 },
              { .type = e_commodity::ore, .quantity = 1 },
              { .type = e_commodity::silver, .quantity = 1 },
              { .type = e_commodity::horses, .quantity = 1 },
              { .type = e_commodity::rum, .quantity = 1 },
              { .type = e_commodity::cigars, .quantity = 1 },
              { .type = e_commodity::cloth, .quantity = 1 },
              { .type = e_commodity::coats, .quantity = 1 },
              { .type     = e_commodity::trade_goods,
                .quantity = 1 },
              { .type = e_commodity::tools, .quantity = 1 },
              { .type = e_commodity::muskets, .quantity = 1 },
          } } };

  REQUIRE( ev.notifications == expected );
}

TEST_CASE( "[colony-evolve] ran out of raw materials" ) {
  World   W;
  Colony& colony = W.add_colony( Coord{ .x = 1, .y = 1 } );
  // Add this so no one starves.
  colony.commodities[e_commodity::food] = 2;

  W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                      e_unit_type::free_colonist );
  W.add_unit_indoors( colony.id, e_indoor_job::tools,
                      e_unit_type::free_colonist );
  colony.buildings[e_colony_building::armory] = true;
  W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                      e_unit_type::free_colonist );

  ColonyEvolution ev = evolve_colony_one_turn(
      W.ss(), W.ts(), W.default_player(), colony );

  vector<ColonyNotification_t> const expected = {
      ColonyNotification::run_out_of_raw_material{
          .what = e_commodity::lumber,
          .job  = e_indoor_job::hammers },
      ColonyNotification::run_out_of_raw_material{
          .what = e_commodity::ore, .job = e_indoor_job::tools },
      ColonyNotification::run_out_of_raw_material{
          .what = e_commodity::tools,
          .job  = e_indoor_job::muskets },
  };

  REQUIRE( ev.notifications == expected );
  REQUIRE_FALSE( ev.colony_disappeared );
}

TEST_CASE( "[colony-evolve] colony starves" ) {
  World W;
  // This way it is impossible for any food to be produced either
  // on the center square or any other square when on a hard dif-
  // ficulty level.
  W.create_arctic_map();
  Colony& colony =
      W.add_colony_with_new_unit( { .x = 1, .y = 1 } );
  REQUIRE( colony.commodities[e_commodity::food] == 0 );
  REQUIRE( colony_units_all( colony ).size() == 1 );

  SECTION( "discoverer" ) {
    // On discoverer the center tile should produce two food on
    // arctic tiles, but the colonist added (on land) will not
    // produce any because it is arctic.
    W.settings().difficulty = e_difficulty::discoverer;
    ColonyEvolution ev      = evolve_colony_one_turn(
        W.ss(), W.ts(), W.default_player(), colony );
    REQUIRE( !ev.colony_disappeared );
  }

  SECTION( "explorer" ) {
    // On explorer and higher the center tile should produce no
    // food on arctic, and neither will the colonist added (on
    // land).
    W.settings().difficulty = e_difficulty::explorer;
    ColonyEvolution ev      = evolve_colony_one_turn(
        W.ss(), W.ts(), W.default_player(), colony );
    REQUIRE( ev.colony_disappeared );
  }

  SECTION( "viceroy" ) {
    W.settings().difficulty = e_difficulty::viceroy;
    ColonyEvolution ev      = evolve_colony_one_turn(
        W.ss(), W.ts(), W.default_player(), colony );
    REQUIRE( ev.colony_disappeared );
  }

  // No matter what, the colony should not have been changed.
  REQUIRE( colony_units_all( colony ).size() == 1 );
}

TEST_CASE(
    "[colony-evolve] does not promote unit producing nothing" ) {
  World   W;
  Colony& colony = W.add_colony( Coord{ .x = 1, .y = 1 } );
  // Make sure no one starves.
  colony.commodities[e_commodity::food] = 100;

  // tries=yes, succeeds=yes. This colonist should be produing
  // nothing, since grassland does not produce cotton.
  W.add_unit_outdoors( colony.id, e_direction::n,
                       e_outdoor_job::cotton,
                       e_unit_type::petty_criminal );
  EXPECT_CALL( W.rand(),
               bernoulli( Approx( 0.00333, 0.00001 ) ) )
      .returns( true );

  // Sanity check. Note unit ids start at 1.
  REQUIRE( W.ss().units.unit_for( 1 ).type() ==
           e_unit_type::petty_criminal );

  ColonyEvolution const ev = evolve_colony_one_turn(
      W.ss(), W.ts(), W.default_player(), colony );

  vector<ColonyNotification_t> const expected;
  REQUIRE( ev.notifications == expected );

  // Check that the unit has not been changed.
  REQUIRE( W.ss().units.unit_for( 1 ).type() ==
           e_unit_type::petty_criminal );
}

TEST_CASE( "[colony-evolve] promotes units" ) {
  World   W;
  Colony& colony = W.add_colony( Coord{ .x = 1, .y = 1 } );
  // Make sure no one starves.
  colony.commodities[e_commodity::food] = 100;

  // For each of the below we need to make sure that the square
  // that the colonist is on is actually producing what we assign
  // it, otherwise it wouldn't be promoted anyway and then we
  // wouldn't be testing what we think.

  // tries=yes, succeeds=yes.
  W.square( W.kColonySquare.moved( e_direction::nw ) ).surface =
      e_surface::land;
  W.square( W.kColonySquare.moved( e_direction::nw ) ).ground =
      e_ground_terrain::savannah;
  W.add_unit_outdoors( colony.id, e_direction::nw,
                       e_outdoor_job::sugar,
                       e_unit_type::petty_criminal );
  EXPECT_CALL( W.rand(),
               bernoulli( Approx( 0.00333, 0.00001 ) ) )
      .returns( true );

  // tries=no.
  W.square( W.kColonySquare.moved( e_direction::n ) ).surface =
      e_surface::land;
  W.square( W.kColonySquare.moved( e_direction::n ) ).overlay =
      e_land_overlay::forest;
  W.add_unit_outdoors( colony.id, e_direction::n,
                       e_outdoor_job::lumber,
                       e_unit_type::petty_criminal );

  // tries=yes, succeeds=no.
  W.square( W.kColonySquare.moved( e_direction::ne ) ).surface =
      e_surface::land;
  W.square( W.kColonySquare.moved( e_direction::ne ) ).ground =
      e_ground_terrain::grassland;
  W.add_unit_outdoors( colony.id, e_direction::ne,
                       e_outdoor_job::tobacco,
                       e_unit_type::indentured_servant );
  EXPECT_CALL( W.rand(), bernoulli( Approx( 0.005, 0.00001 ) ) )
      .returns( false );

  // tries=no.
  W.square( W.kColonySquare.moved( e_direction::w ) ).surface =
      e_surface::land;
  W.square( W.kColonySquare.moved( e_direction::w ) ).ground =
      e_ground_terrain::plains;
  W.add_unit_outdoors( colony.id, e_direction::w,
                       e_outdoor_job::food,
                       e_unit_type::indentured_servant );

  // tries=yes, succeeds=yes.
  W.square( W.kColonySquare.moved( e_direction::e ) ).surface =
      e_surface::land;
  W.square( W.kColonySquare.moved( e_direction::e ) ).ground =
      e_ground_terrain::prairie;
  W.add_unit_outdoors( colony.id, e_direction::e,
                       e_outdoor_job::cotton,
                       e_unit_type::free_colonist );
  EXPECT_CALL( W.rand(), bernoulli( Approx( 0.01, 0.00001 ) ) )
      .returns( true );

  // Sanity check. Note unit ids start at 1.
  REQUIRE( W.ss().units.unit_for( 1 ).type() ==
           e_unit_type::petty_criminal );
  REQUIRE( W.ss().units.unit_for( 2 ).type() ==
           e_unit_type::petty_criminal );
  REQUIRE( W.ss().units.unit_for( 3 ).type() ==
           e_unit_type::indentured_servant );
  REQUIRE( W.ss().units.unit_for( 4 ).type() ==
           e_unit_type::indentured_servant );
  REQUIRE( W.ss().units.unit_for( 5 ).type() ==
           e_unit_type::free_colonist );

  // Here we don't have to test the logic that decides which
  // kinds of units to promote, since that is tested in the
  // promotion module; we just need to test that the units
  // actually get their type changed and that the proper
  // notifications are added.

  ColonyEvolution const ev = evolve_colony_one_turn(
      W.ss(), W.ts(), W.default_player(), colony );

  // Sanity check to make sure that all colonists are producing,
  // since otherwise they wouldn't be promoted anyway.
  using SP = SquareProduction;
  using LP = refl::enum_map<e_direction, SP>;
  REQUIRE(
      ev.production.land_production ==
      LP{ { e_direction::nw,
            SP{ .what = e_outdoor_job::sugar, .quantity = 3 } },
          { e_direction::n,
            SP{ .what = e_outdoor_job::lumber, .quantity = 6 } },
          { e_direction::ne, SP{ .what = e_outdoor_job::tobacco,
                                 .quantity = 3 } },
          { e_direction::w,
            SP{ .what = e_outdoor_job::food, .quantity = 5 } },
          { e_direction::e, SP{ .what = e_outdoor_job::cotton,
                                .quantity = 3 } } } );

  vector<ColonyNotification_t> const expected = {
      ColonyNotification::unit_promoted{
          .promoted_to = e_unit_type::expert_sugar_planter },
      ColonyNotification::unit_promoted{
          .promoted_to = e_unit_type::expert_cotton_planter },
  };

  REQUIRE( ev.notifications == expected );

  // Check that the unit types have actually been changed, but
  // only those that were promoted.
  REQUIRE( W.ss().units.unit_for( 1 ).type() ==
           e_unit_type::expert_sugar_planter );
  REQUIRE( W.ss().units.unit_for( 2 ).type() ==
           e_unit_type::petty_criminal );
  REQUIRE( W.ss().units.unit_for( 3 ).type() ==
           e_unit_type::indentured_servant );
  REQUIRE( W.ss().units.unit_for( 4 ).type() ==
           e_unit_type::indentured_servant );
  REQUIRE( W.ss().units.unit_for( 5 ).type() ==
           e_unit_type::expert_cotton_planter );
}

TEST_CASE( "[colony-evolve] applies production" ) {
  World W;
  // TODO
}

TEST_CASE( "[colony-evolve] construction" ) {
  World W;
  // TODO

  // TODO: test a case where a construction project requires
  // tools and there are just the right amount of tools in the
  // colony but there is also a gunsmith consuming tools. In the
  // original game, the gunsmith will consume the tools before
  // they can be used for the construction project.
}

TEST_CASE( "[colony-evolve] new colonist" ) {
  World W;
  // TODO
}

TEST_CASE( "[colony-evolve] colonist starved" ) {
  World W;
  // TODO
}

TEST_CASE( "[colony-evolve] evolves sons of liberty" ) {
  World W;
  // TODO
}

TEST_CASE( "[colony-evolve] evolves founding father bells" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
