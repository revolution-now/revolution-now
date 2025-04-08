/****************************************************************
**revolution-status-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-13.
*
* Description: Unit tests for the revolution-status module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/revolution-status.hpp"

// Testing.
#include "test/fake/world.hpp"

// ss
#include "src/ss/player.rds.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  World() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      _, L, L, //
      L, L, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[revolution-status] nation_possessive" ) {
  World w;

  Player& player = w.default_player();

  auto f = [&] { return nation_possessive( player ); };

  REQUIRE( f() == "Dutch" );

  player.revolution.status = e_revolution_status::declared;
  REQUIRE( f() == "Rebel" );
}

TEST_CASE( "[revolution-status] nation_display_name" ) {
  World w;

  Player& player = w.default_player();

  auto f = [&] { return nation_display_name( player ); };

  REQUIRE( f() == "Dutch" );

  player.revolution.status = e_revolution_status::declared;
  REQUIRE( f() == "Rebels" );
}

} // namespace
} // namespace rn
