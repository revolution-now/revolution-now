/****************************************************************
**intervention-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-11.
*
* Description: Unit tests for the intervention module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/intervention.hpp"

// Testing.
#include "test/fake/world.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[intervention] bells_required_for_intervention" ) {
  world w;
}

TEST_CASE( "[intervention] select_nation_for_intervention" ) {
  world w;
}

TEST_CASE( "[intervention] should_trigger_intervention" ) {
  world w;
}

TEST_CASE( "[intervention] trigger_intervention" ) {
  world w;
}

TEST_CASE( "[intervention] pick_forces_to_deploy" ) {
  world w;
}

TEST_CASE( "[intervention] find_intervention_deploy_tile" ) {
  world w;
}

TEST_CASE( "[intervention] deploy_intervention_forces" ) {
  world w;
}

TEST_CASE( "[intervention] intervention_forces_intro_ui_seq" ) {
  world w;
}

TEST_CASE(
    "[intervention] intervention_forces_triggered_ui_seq" ) {
  world w;
}

TEST_CASE(
    "[intervention] intervention_forces_deployed_ui_seq" ) {
  world w;
}

TEST_CASE(
    "[intervention] "
    "animate_move_intervention_units_into_colony" ) {
  world w;
}

TEST_CASE(
    "[intervention] move_intervention_units_into_colony" ) {
  world w;
}

} // namespace
} // namespace rn
