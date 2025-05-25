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
TEST_CASE( "[turn-mgr] find_first_player_to_move" ) {
  world w;

  auto const f = [&] {
    return find_first_player_to_move( w.ss().as_const );
  };

  using enum e_player;

  // Default.
  REQUIRE( f() == nothing );

  SECTION( "one player, first" ) {
    w.add_player( e_player::english );
    REQUIRE( f() == english );
  }

  SECTION( "one player, not first" ) {
    w.add_player( e_player::french );
    REQUIRE( f() == french );
  }

  SECTION( "one player, last" ) {
    w.add_player( e_player::dutch );
    REQUIRE( f() == dutch );
  }

  SECTION( "two players, first" ) {
    w.add_player( e_player::english );
    w.add_player( e_player::french );
    REQUIRE( f() == english );
  }

  SECTION( "two players, not first" ) {
    w.add_player( e_player::french );
    w.add_player( e_player::dutch );
    REQUIRE( f() == french );
  }

  SECTION( "two players, last" ) {
    w.add_player( e_player::spanish );
    w.add_player( e_player::dutch );
    REQUIRE( f() == spanish );
  }

  SECTION( "two players, some ref" ) {
    w.add_player( e_player::ref_french );
    w.add_player( e_player::ref_dutch );
    REQUIRE( f() == ref_french );
  }

  SECTION( "all players" ) {
    w.add_player( e_player::english );
    w.add_player( e_player::french );
    w.add_player( e_player::spanish );
    w.add_player( e_player::dutch );
    REQUIRE( f() == english );
  }
}

TEST_CASE( "[turn-mgr] find_next_player_to_move" ) {
  world w;

  auto const f = [&]( e_player const player ) {
    return find_next_player_to_move( w.ss().as_const, player );
  };

  using enum e_player;

  SECTION( "no players" ) {
    REQUIRE( f( english ) == nothing );
    REQUIRE( f( french ) == nothing );
    REQUIRE( f( spanish ) == nothing );
    REQUIRE( f( dutch ) == nothing );
    REQUIRE( f( ref_english ) == nothing );
    REQUIRE( f( ref_french ) == nothing );
    REQUIRE( f( ref_spanish ) == nothing );
    REQUIRE( f( ref_dutch ) == nothing );
  }

  SECTION( "one player, first" ) {
    w.add_player( e_player::english );
    REQUIRE( f( english ) == nothing );
    REQUIRE( f( french ) == nothing );
    REQUIRE( f( spanish ) == nothing );
    REQUIRE( f( dutch ) == nothing );
    REQUIRE( f( ref_english ) == nothing );
    REQUIRE( f( ref_french ) == nothing );
    REQUIRE( f( ref_spanish ) == nothing );
    REQUIRE( f( ref_dutch ) == nothing );
  }

  SECTION( "one player, not first" ) {
    w.add_player( e_player::french );
    REQUIRE( f( english ) == french );
    REQUIRE( f( french ) == nothing );
    REQUIRE( f( spanish ) == nothing );
    REQUIRE( f( dutch ) == nothing );
    REQUIRE( f( ref_english ) == nothing );
    REQUIRE( f( ref_french ) == nothing );
    REQUIRE( f( ref_spanish ) == nothing );
    REQUIRE( f( ref_dutch ) == nothing );
  }

  SECTION( "one player, last" ) {
    w.add_player( e_player::dutch );
    REQUIRE( f( english ) == dutch );
    REQUIRE( f( french ) == dutch );
    REQUIRE( f( spanish ) == dutch );
    REQUIRE( f( dutch ) == nothing );
    REQUIRE( f( ref_english ) == nothing );
    REQUIRE( f( ref_french ) == nothing );
    REQUIRE( f( ref_spanish ) == nothing );
    REQUIRE( f( ref_dutch ) == nothing );
  }

  SECTION( "two players, first" ) {
    w.add_player( e_player::english );
    w.add_player( e_player::french );
    REQUIRE( f( english ) == french );
    REQUIRE( f( french ) == nothing );
    REQUIRE( f( spanish ) == nothing );
    REQUIRE( f( dutch ) == nothing );
    REQUIRE( f( ref_english ) == nothing );
    REQUIRE( f( ref_french ) == nothing );
    REQUIRE( f( ref_spanish ) == nothing );
    REQUIRE( f( ref_dutch ) == nothing );
  }

  SECTION( "two players, not first" ) {
    w.add_player( e_player::french );
    w.add_player( e_player::dutch );
    REQUIRE( f( english ) == french );
    REQUIRE( f( french ) == dutch );
    REQUIRE( f( spanish ) == dutch );
    REQUIRE( f( dutch ) == nothing );
    REQUIRE( f( ref_english ) == nothing );
    REQUIRE( f( ref_french ) == nothing );
    REQUIRE( f( ref_spanish ) == nothing );
    REQUIRE( f( ref_dutch ) == nothing );
  }

  SECTION( "two players with ref, not first" ) {
    w.add_player( e_player::french );
    w.add_player( e_player::dutch );
    w.add_player( e_player::ref_spanish );
    REQUIRE( f( english ) == french );
    REQUIRE( f( french ) == dutch );
    REQUIRE( f( spanish ) == dutch );
    REQUIRE( f( dutch ) == ref_spanish );
    REQUIRE( f( ref_english ) == ref_spanish );
    REQUIRE( f( ref_french ) == ref_spanish );
    REQUIRE( f( ref_spanish ) == nothing );
    REQUIRE( f( ref_dutch ) == nothing );
  }

  SECTION( "two players, last" ) {
    w.add_player( e_player::spanish );
    w.add_player( e_player::dutch );
    REQUIRE( f( english ) == spanish );
    REQUIRE( f( french ) == spanish );
    REQUIRE( f( spanish ) == dutch );
    REQUIRE( f( dutch ) == nothing );
    REQUIRE( f( ref_english ) == nothing );
    REQUIRE( f( ref_french ) == nothing );
    REQUIRE( f( ref_spanish ) == nothing );
    REQUIRE( f( ref_dutch ) == nothing );
  }

  SECTION( "three players" ) {
    w.add_player( e_player::english );
    w.add_player( e_player::spanish );
    w.add_player( e_player::dutch );
    REQUIRE( f( english ) == spanish );
    REQUIRE( f( french ) == spanish );
    REQUIRE( f( spanish ) == dutch );
    REQUIRE( f( dutch ) == nothing );
    REQUIRE( f( ref_english ) == nothing );
    REQUIRE( f( ref_french ) == nothing );
    REQUIRE( f( ref_spanish ) == nothing );
    REQUIRE( f( ref_dutch ) == nothing );
  }

  SECTION( "all players" ) {
    w.add_player( e_player::english );
    w.add_player( e_player::french );
    w.add_player( e_player::spanish );
    w.add_player( e_player::dutch );
    w.add_player( e_player::ref_english );
    w.add_player( e_player::ref_french );
    w.add_player( e_player::ref_spanish );
    w.add_player( e_player::ref_dutch );
    REQUIRE( f( english ) == french );
    REQUIRE( f( french ) == spanish );
    REQUIRE( f( spanish ) == dutch );
    REQUIRE( f( dutch ) == ref_english );
    REQUIRE( f( ref_english ) == ref_french );
    REQUIRE( f( ref_french ) == ref_spanish );
    REQUIRE( f( ref_spanish ) == ref_dutch );
    REQUIRE( f( ref_dutch ) == nothing );
  }
}

} // namespace
} // namespace rn
