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

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

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
TEST_CASE( "[colony-evolve] spoilage" ) {
  World   W;
  Colony& colony = W.add_colony( { .x = 1, .y = 1 } );
  for( e_commodity c : refl::enum_values<e_commodity> )
    colony.commodities[c] = 101;
  colony.commodities[e_commodity::food] = 150;
  ColonyEvolution ev =
      evolve_colony_one_turn( W.ss(), W.ts(), colony );
  REQUIRE( ev.production.center_food_production == 5 );
  // Food is still 150 despite the 5 food produced in the center
  // square because of horse production, which coincidentally
  // happens to allow 5 horses (101+24)/25=5 (each consuming one
  // of the 5 surplus food). That said, the increase in horses
  // won't be reflected in any of the numbers below because the
  // new horses will be suppressed because the number of horses
  // is already over capacity (so the number of horses that will
  // be reported as having spoiled is actually only 1).
  REQUIRE( colony.commodities[e_commodity::food] == 150 );
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
  ev = evolve_colony_one_turn( W.ss(), W.ts(), colony );
  REQUIRE( ev.production.center_food_production == 5 );
  // Food goes up by 5 due to center square production and down
  // by 5 due to horse breeding, then down 200 due to new
  // colonist being produced, putting the final value at 600. See
  // corresponding comment above for more info on the horse
  // breeding.
  REQUIRE( colony.commodities[e_commodity::food] == 600 );
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
  W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                      e_unit_type::free_colonist );

  using SP  = SquareProduction;
  using LP  = refl::enum_map<e_direction, SP>;
  using RMP = RawMaterialAndProduct;

  ColonyEvolution ev =
      evolve_colony_one_turn( W.ss(), W.ts(), colony );

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
}

TEST_CASE( "[colony-evolve] applies production" ) {
  World W;
  // TODO
}

TEST_CASE( "[colony-evolve] construction" ) {
  World W;
  // TODO
}

TEST_CASE( "[colony-evolve] new colonist" ) {
  World W;
  // TODO
}

TEST_CASE( "[colony-evolve] colonist starved" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
