/****************************************************************
**classic-sav-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-13.
*
* Description: Unit tests for the classic-sav module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/classic-sav.hpp"

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
TEST_CASE( "[classic-sav] load_classic_map_file" ) {
  World W;
}

} // namespace
} // namespace rn
