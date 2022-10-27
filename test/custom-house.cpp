/****************************************************************
**custom-house.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-22.
*
* Description: Unit tests for the src/custom-house.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/custom-house.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/igui.hpp"

// refl
#include "refl/to-str.hpp"

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
    create_default_map();
    add_default_player();
  }

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[custom-house] set_default_custom_house_state" ) {
  World   W;
  Colony& colony = W.add_colony_with_new_unit( Coord{} );

  // Sanity check.
  for( auto& [comm, on] : colony.custom_house ) {
    INFO( fmt::format( "commodity: {}", comm ) );
    REQUIRE( !on );
  }

  set_default_custom_house_state( colony );

  REQUIRE_FALSE( colony.custom_house[e_commodity::food] );
  REQUIRE( colony.custom_house[e_commodity::sugar] );
  REQUIRE( colony.custom_house[e_commodity::tobacco] );
  REQUIRE( colony.custom_house[e_commodity::cotton] );
  REQUIRE( colony.custom_house[e_commodity::fur] );
  REQUIRE_FALSE( colony.custom_house[e_commodity::lumber] );
  REQUIRE( colony.custom_house[e_commodity::ore] );
  REQUIRE( colony.custom_house[e_commodity::silver] );
  REQUIRE_FALSE( colony.custom_house[e_commodity::horses] );
  REQUIRE( colony.custom_house[e_commodity::rum] );
  REQUIRE( colony.custom_house[e_commodity::cigars] );
  REQUIRE( colony.custom_house[e_commodity::cloth] );
  REQUIRE( colony.custom_house[e_commodity::coats] );
  REQUIRE_FALSE( colony.custom_house[e_commodity::trade_goods] );
  REQUIRE_FALSE( colony.custom_house[e_commodity::tools] );
  REQUIRE_FALSE( colony.custom_house[e_commodity::muskets] );
}

} // namespace
} // namespace rn
