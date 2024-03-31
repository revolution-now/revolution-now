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
#include "test/mocks/irand.hpp"

// ss
#include "src/ss/map.hpp"
#include "src/ss/ref.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

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
    MapSquare const   L = make_grassland();
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
  World                  W;
  vector<DepletionEvent> expected;
  using R = e_natural_resource;
  using O = e_land_overlay;

  // C = colony              L, L, L, L, L,
  // S = undepleted silver   L, L, L, L, L,
  // s = depleted silver     L, T, L, S, L,
  // D = deer/game           L, M, C, s, L,
  // M = minerals            L, D, M, S, L,
  // T = tobacco             L, L, L, L, L,

  Colony& colony = W.add_colony( { .x = 2, .y = 3 } );

  W.square( { .x = 1, .y = 4 } ).overlay = O::forest;
  W.square( { .x = 2, .y = 4 } ).overlay = O::forest;

  W.square( { .x = 1, .y = 2 } ).ground_resource = R::tobacco;
  W.square( { .x = 3, .y = 2 } ).ground_resource = R::silver;
  W.square( { .x = 1, .y = 3 } ).ground_resource = R::minerals;
  W.square( { .x = 1, .y = 4 } ).forest_resource = R::deer;
  W.square( { .x = 2, .y = 4 } ).ground_resource = R::cotton;
  W.square( { .x = 2, .y = 4 } ).forest_resource = R::minerals;
  W.square( { .x = 3, .y = 4 } ).ground_resource = R::silver;
  W.square( { .x = 3, .y = 3 } ).ground_resource =
      R::silver_depleted;

  auto f = [&] {
    return advance_depletion_state( W.ss(), W.rand(), colony );
  };

  expected = {};
  REQUIRE( f() == expected );
}

TEST_CASE( "[depletion] update_depleted_tiles" ) {
  World W;
}

TEST_CASE( "[depletion] remove_depletion_counter_if_needed" ) {
  World W;
}

TEST_CASE(
    "[depletion] remove_depletion_counters_where_needed" ) {
  World W;

  auto f = [&] {
    remove_depletion_counters_where_needed( W.ss() );
  };

  REQUIRE( W.map().depletion.counters.empty() );
  W.map().depletion.counters[{ .x = 1, .y = 2 }] = 5;
  W.map().depletion.counters[{ .x = 1, .y = 1 }] = 2;
  REQUIRE( W.map().depletion.counters.size() == 2 );
  f();
  REQUIRE( W.map().depletion.counters.empty() );
}

} // namespace
} // namespace rn
