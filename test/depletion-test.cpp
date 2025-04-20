/****************************************************************
**depletion-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-29.
*
* Description: Unit tests for the depletion module.
*
*****************************************************************/
#include "terrain-enums.rds.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/depletion.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/map-square.hpp"

// ss
#include "src/ss/difficulty.rds.hpp"
#include "src/ss/map.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/terrain.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

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

  void create_default_map() {
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, L, L, //
      L, L, L, L, L, //
      L, L, L, L, L, //
      L, L, L, L, L, //
      L, L, L, L, L, //
      L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 5 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[depletion] advance_depletion_state" ) {
  World W;
  vector<DepletionEvent> expected;
  ResourceDepletion expected_depletion;
  using R = e_natural_resource;
  using O = e_land_overlay;

  auto& depletion = W.map().depletion;

  auto ground = [&]( Coord tile ) -> auto& {
    return W.square( tile ).ground_resource;
  };

  auto forest = [&]( Coord tile ) -> auto& {
    return W.square( tile ).forest_resource;
  };

  // C = colony              L, L, L, L,C2,
  // S = undepleted silver   L, L, L, M, S,
  // s = depleted silver     L, T, L, S, L,
  // D = deer/game           L, M, C1,s, L,
  // M = minerals            L, D, M, S, L,
  // T = tobacco             L, L, L, L, L,

  Colony& colony1 = W.add_colony( { .x = 2, .y = 3 } );
  Colony& colony2 = W.add_colony( { .x = 4, .y = 0 } );

  W.square( { .x = 1, .y = 4 } ).overlay = O::forest;
  W.square( { .x = 2, .y = 4 } ).overlay = O::forest;
  W.square( { .x = 3, .y = 1 } ).overlay = O::forest;

  ground( { .x = 1, .y = 2 } ) = R::tobacco;
  ground( { .x = 3, .y = 2 } ) = R::silver;
  ground( { .x = 1, .y = 3 } ) = R::minerals;
  forest( { .x = 1, .y = 4 } ) = R::deer;
  ground( { .x = 2, .y = 4 } ) = R::cotton;
  forest( { .x = 2, .y = 4 } ) = R::minerals;
  ground( { .x = 3, .y = 4 } ) = R::silver;
  ground( { .x = 3, .y = 3 } ) = R::silver_depleted;
  ground( { .x = 3, .y = 1 } ) = R::silver;
  forest( { .x = 3, .y = 1 } ) = R::minerals;
  ground( { .x = 4, .y = 1 } ) = R::silver;

  W.settings().game_setup_options.difficulty =
      e_difficulty::conquistador;

  auto f1 = [&] {
    return advance_depletion_state( W.ss(), W.rand(), colony1 );
  };

  auto f2 = [&] {
    return advance_depletion_state( W.ss(), W.rand(), colony2 );
  };

  expected           = {};
  expected_depletion = {};
  REQUIRE( f1() == expected );
  REQUIRE( depletion == expected_depletion );
  REQUIRE( f2() == expected );
  REQUIRE( depletion == expected_depletion );

  // Colony on edge of map. Initially the jobs are swapped.
  W.add_unit_outdoors( colony2.id, e_direction::sw,
                       e_outdoor_job::tobacco );
  W.add_unit_outdoors( colony2.id, e_direction::w,
                       e_outdoor_job::silver );
  expected           = {};
  expected_depletion = {};
  REQUIRE( f2() == expected );
  REQUIRE( depletion == expected_depletion );

  // Swap the jobs.
  colony2.outdoor_jobs[e_direction::sw]->job =
      e_outdoor_job::silver;
  colony2.outdoor_jobs[e_direction::w]->job =
      e_outdoor_job::tobacco;
  W.rand().EXPECT__bernoulli( .75 ).returns( false );
  expected           = {};
  expected_depletion = {};
  REQUIRE( f2() == expected );
  REQUIRE( depletion == expected_depletion );

  W.rand().EXPECT__bernoulli( .75 ).returns( true );
  expected           = {};
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 2 },
                         } };
  REQUIRE( f2() == expected );
  REQUIRE( depletion == expected_depletion );

  W.rand().EXPECT__bernoulli( .75 ).returns( true );
  expected           = {};
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 4 },
                         } };
  REQUIRE( f2() == expected );
  REQUIRE( depletion == expected_depletion );

  colony2.outdoor_jobs[e_direction::sw]->job =
      e_outdoor_job::ore;
  W.rand().EXPECT__bernoulli( .75 ).returns( true );
  expected           = {};
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 5 },
                         } };
  REQUIRE( f2() == expected );
  REQUIRE( depletion == expected_depletion );

  W.add_unit_outdoors( colony2.id, e_direction::s,
                       e_outdoor_job::silver );
  W.rand().EXPECT__bernoulli( .75 ).returns( true );
  W.rand().EXPECT__bernoulli( .75 ).returns( true );
  expected           = {};
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 6 },
                           { { .x = 4, .y = 1 }, 1 },
                         } };
  REQUIRE( f2() == expected );
  REQUIRE( depletion == expected_depletion );

  // Add a unit on a tile that does not exist.
  BASE_CHECK( !W.terrain().square_exists(
      colony2.location.moved( e_direction::e ) ) );
  W.add_unit_outdoors( colony2.id, e_direction::e,
                       e_outdoor_job::ore );
  W.rand().EXPECT__bernoulli( .75 ).returns( false );
  W.rand().EXPECT__bernoulli( .75 ).returns( true );
  expected           = {};
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 6 },
                           { { .x = 4, .y = 1 }, 2 },
                         } };
  REQUIRE( f2() == expected );
  REQUIRE( depletion == expected_depletion );

  W.settings().game_setup_options.difficulty =
      e_difficulty::discoverer;

  // Reminder:
  // C = colony              L, L, L, L,C2,
  // S = undepleted silver   L, L, L, M, S,
  // s = depleted silver     L, T, L, S, L,
  // D = deer/game           L, M, C1,s, L,
  // M = minerals            L, D, M, S, L,
  // T = tobacco             L, L, L, L, L,
  expected           = {};
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 6 },
                           { { .x = 4, .y = 1 }, 2 },
                         } };
  REQUIRE( f1() == expected );
  REQUIRE( depletion == expected_depletion );

  // Add units on all tiles except for one.
  W.add_unit_outdoors( colony1.id, e_direction::nw,
                       e_outdoor_job::tobacco );
  W.add_unit_outdoors( colony1.id, e_direction::n,
                       e_outdoor_job::cotton );
  W.add_unit_outdoors( colony1.id, e_direction::ne,
                       e_outdoor_job::silver );
  W.add_unit_outdoors( colony1.id, e_direction::w,
                       e_outdoor_job::ore );
  W.add_unit_outdoors( colony1.id, e_direction::e,
                       e_outdoor_job::silver );
  // This one is important because it tests that we have a unit
  // working on a depletable commodity (silver) on a tile that
  // indeed has a prime resource, but not silver.
  BASE_CHECK(
      effective_resource( W.terrain().square_at(
          colony1.location.moved( e_direction::sw ) ) ) ==
      e_natural_resource::deer );
  W.add_unit_outdoors( colony1.id, e_direction::sw,
                       e_outdoor_job::silver );
  W.add_unit_outdoors( colony1.id, e_direction::se,
                       e_outdoor_job::sugar );
  W.rand().EXPECT__bernoulli( .5 ).returns( true );
  W.rand().EXPECT__bernoulli( .5 ).returns( true );
  expected           = {};
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 6 },
                           { { .x = 4, .y = 1 }, 2 },
                           { { .x = 3, .y = 2 }, 1 },
                           { { .x = 1, .y = 3 }, 1 },
                         } };
  REQUIRE( f1() == expected );
  REQUIRE( depletion == expected_depletion );

  depletion.counters[{ .x = 3, .y = 2 }] = 47;
  depletion.counters[{ .x = 1, .y = 3 }] = 48;
  REQUIRE( effective_resource( W.terrain().square_at(
               colony1.location.moved( e_direction::w ) ) ) ==
           e_natural_resource::minerals );

  W.rand().EXPECT__bernoulli( .5 ).returns( true );
  W.rand().EXPECT__bernoulli( .5 ).returns( true );
  expected           = {};
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 6 },
                           { { .x = 4, .y = 1 }, 2 },
                           { { .x = 3, .y = 2 }, 48 },
                           { { .x = 1, .y = 3 }, 49 },
                         } };
  REQUIRE( f1() == expected );
  REQUIRE( depletion == expected_depletion );
  REQUIRE( effective_resource( W.terrain().square_at(
               colony1.location.moved( e_direction::w ) ) ) ==
           e_natural_resource::minerals );

  W.rand().EXPECT__bernoulli( .5 ).returns( true );
  W.rand().EXPECT__bernoulli( .5 ).returns( true );
  expected           = { DepletionEvent{
              .tile          = { .x = 1, .y = 3 },
              .resource_from = e_natural_resource::minerals,
              .resource_to   = nothing } };
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 6 },
                           { { .x = 4, .y = 1 }, 2 },
                           { { .x = 3, .y = 2 }, 49 },
                         } };
  REQUIRE( f1() == expected );
  REQUIRE( depletion == expected_depletion );
  // This function does not change the terrain.
  REQUIRE( effective_resource( W.terrain().square_at(
               colony1.location.moved( e_direction::w ) ) ) ==
           e_natural_resource::minerals );

  W.rand().EXPECT__bernoulli( .5 ).returns( true );
  W.rand().EXPECT__bernoulli( .5 ).returns( true );
  expected           = { DepletionEvent{
              .tile          = { .x = 3, .y = 2 },
              .resource_from = e_natural_resource::silver,
              .resource_to   = e_natural_resource::silver_depleted } };
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 6 },
                           { { .x = 4, .y = 1 }, 2 },
                           { { .x = 1, .y = 3 }, 1 },
                         } };
  REQUIRE( f1() == expected );
  REQUIRE( depletion == expected_depletion );
  // This function does not change the terrain.
  REQUIRE( effective_resource( W.terrain().square_at(
               colony1.location.moved( e_direction::ne ) ) ) ==
           e_natural_resource::silver );

  W.settings().game_setup_options.difficulty =
      e_difficulty::viceroy;

  depletion.counters[{ .x = 3, .y = 2 }] = 48;
  depletion.counters[{ .x = 1, .y = 3 }] = 48;
  W.rand()
      .EXPECT__bernoulli( Approx( .833333, .00001 ) )
      .returns( true );
  W.rand()
      .EXPECT__bernoulli( Approx( .833333, .00001 ) )
      .returns( true );
  expected           = {};
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 6 },
                           { { .x = 4, .y = 1 }, 2 },
                           { { .x = 3, .y = 2 }, 49 },
                           { { .x = 1, .y = 3 }, 49 },
                         } };
  REQUIRE( f1() == expected );
  REQUIRE( depletion == expected_depletion );

  W.rand()
      .EXPECT__bernoulli( Approx( .833333, .00001 ) )
      .returns( false );
  W.rand()
      .EXPECT__bernoulli( Approx( .833333, .00001 ) )
      .returns( false );
  expected           = {};
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 6 },
                           { { .x = 4, .y = 1 }, 2 },
                           { { .x = 3, .y = 2 }, 49 },
                           { { .x = 1, .y = 3 }, 49 },
                         } };
  REQUIRE( f1() == expected );
  REQUIRE( depletion == expected_depletion );

  // Two depletion events at once.
  W.rand()
      .EXPECT__bernoulli( Approx( .833333, .00001 ) )
      .returns( true );
  W.rand()
      .EXPECT__bernoulli( Approx( .833333, .00001 ) )
      .returns( true );
  expected = {
    { .tile          = { .x = 3, .y = 2 },
      .resource_from = e_natural_resource::silver,
      .resource_to   = e_natural_resource::silver_depleted },
    { .tile          = { .x = 1, .y = 3 },
      .resource_from = e_natural_resource::minerals,
      .resource_to   = nothing },
  };
  expected_depletion = { .counters = {
                           { { .x = 3, .y = 1 }, 6 },
                           { { .x = 4, .y = 1 }, 2 },
                         } };
  REQUIRE( f1() == expected );
  REQUIRE( depletion == expected_depletion );
}

TEST_CASE( "[depletion] update_depleted_tiles" ) {
  World W;
  vector<DepletionEvent> events;

  auto f = [&] {
    update_depleted_tiles( W.map_updater(), events );
  };

  auto ground = [&]( Coord tile ) -> auto& {
    return W.square( tile ).ground_resource;
  };

  auto forest = [&]( Coord tile ) -> auto& {
    return W.square( tile ).forest_resource;
  };

  //                         L, L, L, L, L,
  // S = undepleted silver   L, L, L, M, S,
  // s = depleted silver     L, T, L, S, L,
  // D = deer/game           L, M, L, s, L,
  // M = minerals            L, D, M, S, L,
  // T = tobacco             L, L, L, L, L,

  using R                                = e_natural_resource;
  using O                                = e_land_overlay;
  W.square( { .x = 1, .y = 4 } ).overlay = O::forest;
  W.square( { .x = 2, .y = 4 } ).overlay = O::forest;
  W.square( { .x = 3, .y = 1 } ).overlay = O::forest;

  ground( { .x = 1, .y = 2 } ) = R::tobacco;
  ground( { .x = 3, .y = 2 } ) = R::silver;
  ground( { .x = 1, .y = 3 } ) = R::minerals;
  forest( { .x = 1, .y = 4 } ) = R::deer;
  ground( { .x = 2, .y = 4 } ) = R::cotton;
  forest( { .x = 2, .y = 4 } ) = R::minerals;
  ground( { .x = 3, .y = 4 } ) = R::silver;
  ground( { .x = 3, .y = 3 } ) = R::silver_depleted;
  ground( { .x = 3, .y = 1 } ) = R::silver;
  forest( { .x = 3, .y = 1 } ) = R::minerals;
  ground( { .x = 4, .y = 1 } ) = R::silver;

  events = {};
  f();

  REQUIRE( W.square( { .x = 3, .y = 2 } ).ground_resource ==
           R::silver );
  REQUIRE( W.square( { .x = 3, .y = 2 } ).forest_resource ==
           nothing );
  REQUIRE( W.square( { .x = 2, .y = 4 } ).ground_resource ==
           R::cotton );
  REQUIRE( W.square( { .x = 2, .y = 4 } ).forest_resource ==
           R::minerals );

  events = {
    DepletionEvent{ .tile          = { .x = 3, .y = 2 },
                    .resource_from = R::silver,
                    .resource_to   = R::silver_depleted },
    DepletionEvent{ .tile          = { .x = 2, .y = 4 },
                    .resource_from = R::minerals,
                    .resource_to   = nothing },
  };
  f();
  REQUIRE( W.square( { .x = 3, .y = 2 } ).ground_resource ==
           R::silver_depleted );
  REQUIRE( W.square( { .x = 3, .y = 2 } ).forest_resource ==
           nothing );
  REQUIRE( W.square( { .x = 2, .y = 4 } ).ground_resource ==
           R::cotton );
  REQUIRE( W.square( { .x = 2, .y = 4 } ).forest_resource ==
           nothing );
}

TEST_CASE( "[depletion] remove_depletion_counter_if_needed" ) {
  World W;

  auto& counters = W.map().depletion.counters;
  auto expected  = counters;

  using R = e_natural_resource;
  using O = e_land_overlay;

  auto f = [&]( Coord tile ) {
    remove_depletion_counter_if_needed( W.ss(), tile );
  };

  REQUIRE( W.map().depletion.counters.empty() );
  W.map().depletion.counters[{ .x = 1, .y = 2 }] = 5;
  W.map().depletion.counters[{ .x = 1, .y = 1 }] = 2;
  W.square( { .x = 1, .y = 2 } ).ground_resource = R::silver;
  W.square( { .x = 1, .y = 1 } ).ground_resource = R::cotton;
  W.square( { .x = 1, .y = 1 } ).forest_resource = R::minerals;
  W.square( { .x = 1, .y = 1 } ).overlay         = O::forest;
  REQUIRE( W.map().depletion.counters.size() == 2 );

  expected = { { { .x = 1, .y = 2 }, 5 },
               { { .x = 1, .y = 1 }, 2 } };
  REQUIRE( counters == expected );

  expected = { { { .x = 1, .y = 2 }, 5 },
               { { .x = 1, .y = 1 }, 2 } };
  f( { .x = 0, .y = 0 } );
  REQUIRE( counters == expected );

  expected = { { { .x = 1, .y = 2 }, 5 },
               { { .x = 1, .y = 1 }, 2 } };
  f( { .x = 1, .y = 2 } );
  REQUIRE( counters == expected );

  expected = { { { .x = 1, .y = 2 }, 5 },
               { { .x = 1, .y = 1 }, 2 } };
  f( { .x = 1, .y = 1 } );
  REQUIRE( counters == expected );

  W.square( { .x = 1, .y = 1 } ).ground_resource = nothing;
  expected = { { { .x = 1, .y = 2 }, 5 },
               { { .x = 1, .y = 1 }, 2 } };
  f( { .x = 1, .y = 1 } );
  REQUIRE( counters == expected );

  W.square( { .x = 1, .y = 1 } ).forest_resource = nothing;
  expected = { { { .x = 1, .y = 2 }, 5 } };
  f( { .x = 1, .y = 1 } );
  REQUIRE( counters == expected );

  W.square( { .x = 1, .y = 2 } ).forest_resource = nothing;
  expected = { { { .x = 1, .y = 2 }, 5 } };
  f( { .x = 1, .y = 2 } );
  REQUIRE( counters == expected );

  W.square( { .x = 1, .y = 2 } ).ground_resource = nothing;
  expected                                       = {};
  f( { .x = 1, .y = 2 } );
  REQUIRE( counters == expected );
}

TEST_CASE(
    "[depletion] remove_depletion_counters_where_needed" ) {
  World W;

  auto& counters = W.map().depletion.counters;
  auto expected  = counters;

  using R = e_natural_resource;
  using O = e_land_overlay;

  auto f = [&] {
    remove_depletion_counters_where_needed( W.ss() );
  };

  REQUIRE( W.map().depletion.counters.empty() );
  W.map().depletion.counters[{ .x = 1, .y = 2 }] = 5;
  W.map().depletion.counters[{ .x = 1, .y = 3 }] = 6;
  W.map().depletion.counters[{ .x = 1, .y = 1 }] = 2;
  W.square( { .x = 1, .y = 2 } ).ground_resource = R::silver;
  W.square( { .x = 1, .y = 3 } ).ground_resource = R::minerals;
  W.square( { .x = 1, .y = 1 } ).ground_resource = R::cotton;
  W.square( { .x = 1, .y = 1 } ).forest_resource = R::minerals;
  W.square( { .x = 1, .y = 1 } ).overlay         = O::forest;
  REQUIRE( W.map().depletion.counters.size() == 3 );

  expected = { { { .x = 1, .y = 2 }, 5 },
               { { .x = 1, .y = 3 }, 6 },
               { { .x = 1, .y = 1 }, 2 } };
  REQUIRE( counters == expected );

  expected = { { { .x = 1, .y = 2 }, 5 },
               { { .x = 1, .y = 3 }, 6 },
               { { .x = 1, .y = 1 }, 2 } };
  f();
  REQUIRE( counters == expected );

  W.square( { .x = 1, .y = 1 } ).ground_resource = nothing;
  expected = { { { .x = 1, .y = 2 }, 5 },
               { { .x = 1, .y = 3 }, 6 },
               { { .x = 1, .y = 1 }, 2 } };
  f();
  REQUIRE( counters == expected );

  W.square( { .x = 1, .y = 1 } ).forest_resource = nothing;
  expected = { { { .x = 1, .y = 2 }, 5 },
               { { .x = 1, .y = 3 }, 6 } };
  f();
  REQUIRE( counters == expected );

  W.square( { .x = 1, .y = 2 } ).forest_resource = nothing;
  expected = { { { .x = 1, .y = 2 }, 5 },
               { { .x = 1, .y = 3 }, 6 } };
  f();
  REQUIRE( counters == expected );

  W.square( { .x = 1, .y = 2 } ).ground_resource = nothing;
  W.square( { .x = 1, .y = 3 } ).ground_resource = nothing;
  expected                                       = {};
  f();
  REQUIRE( counters == expected );
}

} // namespace
} // namespace rn
