/****************************************************************
**players-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-25.
*
* Description: Unit tests for the ss/players module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/players.hpp"

// Testing
#include "test/fake/world.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::base::valid;
using ::Catch::Matchers::Contains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    add_player( e_player::english );
    add_player( e_player::french );
    set_default_player_type( e_player::french );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
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
TEST_CASE(
    "[ss/players] players have symmetric relationships" ) {
  world w;
  base::valid_or<string> v = valid;

  REQUIRE( w.players().validate() == valid );

  w.players()
      .players[e_player::english]
      ->relationship_with[e_player::spanish] =
      e_euro_relationship::peace;
  v = w.players().validate();
  REQUIRE( v != valid );
  REQUIRE_THAT( v.error(),
                Contains( "player spanish does not exist" ) );

  w.players()
      .players[e_player::english]
      ->relationship_with[e_player::spanish] =
      e_euro_relationship::none;
  REQUIRE( w.players().validate() == valid );

  w.players()
      .players[e_player::english]
      ->relationship_with[e_player::french] =
      e_euro_relationship::peace;
  v = w.players().validate();
  REQUIRE( v != valid );
  REQUIRE_THAT( v.error(),
                Contains( "has assymetric relationship" ) );
}

} // namespace
} // namespace rn
