/****************************************************************
**turn-mgr-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-20.
*
* Description: Unit tests for the turn-mgr module.
*
*****************************************************************/
#include "turn-mgr.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/turn-mgr.hpp"

// Testing.
#include "test/fake/world.hpp"

// ss
#include "src/ss/players.rds.hpp"
#include "src/ss/ref.hpp"

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
    create_default_map();
    // !! Do not add players here.
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
TEST_CASE( "[turn-mgr] find_first_nation_to_move" ) {
  world w;

  auto const f = [&] {
    return find_first_nation_to_move( w.ss().as_const );
  };

  using enum e_nation;

  // Default.
  REQUIRE( f() == nothing );

  SECTION( "one nation, first" ) {
    w.add_player( english );
    REQUIRE( f() == english );
  }

  SECTION( "one nation, not first" ) {
    w.add_player( french );
    REQUIRE( f() == french );
  }

  SECTION( "one nation, last" ) {
    w.add_player( dutch );
    REQUIRE( f() == dutch );
  }

  SECTION( "two nations, first" ) {
    w.add_player( english );
    w.add_player( french );
    REQUIRE( f() == english );
  }

  SECTION( "two nations, not first" ) {
    w.add_player( french );
    w.add_player( dutch );
    REQUIRE( f() == french );
  }

  SECTION( "two nations, last" ) {
    w.add_player( spanish );
    w.add_player( dutch );
    REQUIRE( f() == spanish );
  }

  SECTION( "all nations" ) {
    w.add_player( english );
    w.add_player( french );
    w.add_player( spanish );
    w.add_player( dutch );
    REQUIRE( f() == english );
  }
}

TEST_CASE( "[turn-mgr] find_next_nation_to_move" ) {
  world w;

  auto const f = [&]( e_nation const nation ) {
    return find_next_nation_to_move( w.ss().as_const, nation );
  };

  using enum e_nation;

  SECTION( "no nations" ) {
    REQUIRE( f( english ) == nothing );
    REQUIRE( f( french ) == nothing );
    REQUIRE( f( spanish ) == nothing );
    REQUIRE( f( dutch ) == nothing );
  }

  SECTION( "one nation, first" ) {
    w.add_player( english );
    REQUIRE( f( english ) == nothing );
    REQUIRE( f( french ) == nothing );
    REQUIRE( f( spanish ) == nothing );
    REQUIRE( f( dutch ) == nothing );
  }

  SECTION( "one nation, not first" ) {
    w.add_player( french );
    REQUIRE( f( english ) == french );
    REQUIRE( f( french ) == nothing );
    REQUIRE( f( spanish ) == nothing );
    REQUIRE( f( dutch ) == nothing );
  }

  SECTION( "one nation, last" ) {
    w.add_player( dutch );
    REQUIRE( f( english ) == dutch );
    REQUIRE( f( french ) == dutch );
    REQUIRE( f( spanish ) == dutch );
    REQUIRE( f( dutch ) == nothing );
  }

  SECTION( "two nations, first" ) {
    w.add_player( english );
    w.add_player( french );
    REQUIRE( f( english ) == french );
    REQUIRE( f( french ) == nothing );
    REQUIRE( f( spanish ) == nothing );
    REQUIRE( f( dutch ) == nothing );
  }

  SECTION( "two nations, not first" ) {
    w.add_player( french );
    w.add_player( dutch );
    REQUIRE( f( english ) == french );
    REQUIRE( f( french ) == dutch );
    REQUIRE( f( spanish ) == dutch );
    REQUIRE( f( dutch ) == nothing );
  }

  SECTION( "two nations, last" ) {
    w.add_player( spanish );
    w.add_player( dutch );
    REQUIRE( f( english ) == spanish );
    REQUIRE( f( french ) == spanish );
    REQUIRE( f( spanish ) == dutch );
    REQUIRE( f( dutch ) == nothing );
  }

  SECTION( "three nations" ) {
    w.add_player( english );
    w.add_player( spanish );
    w.add_player( dutch );
    REQUIRE( f( english ) == spanish );
    REQUIRE( f( french ) == spanish );
    REQUIRE( f( spanish ) == dutch );
    REQUIRE( f( dutch ) == nothing );
  }

  SECTION( "all nations" ) {
    w.add_player( english );
    w.add_player( french );
    w.add_player( spanish );
    w.add_player( dutch );
    REQUIRE( f( english ) == french );
    REQUIRE( f( french ) == spanish );
    REQUIRE( f( spanish ) == dutch );
    REQUIRE( f( dutch ) == nothing );
  }
}

} // namespace
} // namespace rn
