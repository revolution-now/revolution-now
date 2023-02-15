/****************************************************************
**terrain.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-15.
*
* Description: Unit tests for the src/terrain.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/terrain.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "ss/terrain.hpp"

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
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, //
        L, L, L, //
        _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[terrain] is_pacific_ocean" ) {
  World W;
  W.terrain().pacific_ocean_endpoints()[0] = 1;
  W.terrain().pacific_ocean_endpoints()[1] = 2;
  W.terrain().pacific_ocean_endpoints()[2] = 0;

  auto p = [&]( int y, int x ) {
    return W.terrain().is_pacific_ocean( { .x = x, .y = y } );
  };

  REQUIRE( p( 0, 0 ) == true );
  REQUIRE( p( 0, 1 ) == false );
  REQUIRE( p( 0, 2 ) == false );
  REQUIRE( p( 1, 0 ) == true );
  REQUIRE( p( 1, 1 ) == true );
  REQUIRE( p( 1, 2 ) == false );
  REQUIRE( p( 2, 0 ) == false );
  REQUIRE( p( 2, 1 ) == false );
  REQUIRE( p( 2, 2 ) == false );
}

} // namespace
} // namespace rn
