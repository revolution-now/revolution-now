/****************************************************************
**player-mgr-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-22.
*
* Description: Unit tests for the player-mgr module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/player-mgr.hpp"

// Testing.
#include "test/fake/world.hpp"

// ss
#include "src/ss/players.hpp"
#include "src/ss/terrain.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() { create_default_map(); }

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
TEST_CASE( "[player-mgr] add_new_player" ) {
  world w;

  using enum e_player;

  REQUIRE( !w.players().players[english] );
  REQUIRE( !w.players().players[french] );
  REQUIRE( !w.players().players[spanish] );
  REQUIRE( !w.players().players[dutch] );
  REQUIRE( !w.players().players[ref_english] );
  REQUIRE( !w.players().players[ref_french] );
  REQUIRE( !w.players().players[ref_spanish] );
  REQUIRE( !w.players().players[ref_dutch] );

  REQUIRE( !w.terrain().player_terrain( english ) );
  REQUIRE( !w.terrain().player_terrain( french ) );
  REQUIRE( !w.terrain().player_terrain( spanish ) );
  REQUIRE( !w.terrain().player_terrain( dutch ) );
  REQUIRE( !w.terrain().player_terrain( ref_english ) );
  REQUIRE( !w.terrain().player_terrain( ref_french ) );
  REQUIRE( !w.terrain().player_terrain( ref_spanish ) );
  REQUIRE( !w.terrain().player_terrain( ref_dutch ) );

  add_new_player( w.ss(), w.ts(), spanish );

  REQUIRE( !w.players().players[english] );
  REQUIRE( !w.players().players[french] );
  REQUIRE( w.players().players[spanish] );
  REQUIRE( !w.players().players[dutch] );
  REQUIRE( !w.players().players[ref_english] );
  REQUIRE( !w.players().players[ref_french] );
  REQUIRE( !w.players().players[ref_spanish] );
  REQUIRE( !w.players().players[ref_dutch] );

  REQUIRE( !w.terrain().player_terrain( english ) );
  REQUIRE( !w.terrain().player_terrain( french ) );
  REQUIRE( w.terrain().player_terrain( spanish ) );
  REQUIRE( !w.terrain().player_terrain( dutch ) );
  REQUIRE( !w.terrain().player_terrain( ref_english ) );
  REQUIRE( !w.terrain().player_terrain( ref_french ) );
  REQUIRE( !w.terrain().player_terrain( ref_spanish ) );
  REQUIRE( !w.terrain().player_terrain( ref_dutch ) );

  REQUIRE( w.players().players[spanish]->type == spanish );
  REQUIRE( w.players().players[spanish]->nation ==
           e_nation::spanish );

  add_new_player( w.ss(), w.ts(), ref_english );

  REQUIRE( !w.players().players[english] );
  REQUIRE( !w.players().players[french] );
  REQUIRE( w.players().players[spanish] );
  REQUIRE( !w.players().players[dutch] );
  REQUIRE( w.players().players[ref_english] );
  REQUIRE( !w.players().players[ref_french] );
  REQUIRE( !w.players().players[ref_spanish] );
  REQUIRE( !w.players().players[ref_dutch] );

  REQUIRE( !w.terrain().player_terrain( english ) );
  REQUIRE( !w.terrain().player_terrain( french ) );
  REQUIRE( w.terrain().player_terrain( spanish ) );
  REQUIRE( !w.terrain().player_terrain( dutch ) );
  REQUIRE( w.terrain().player_terrain( ref_english ) );
  REQUIRE( !w.terrain().player_terrain( ref_french ) );
  REQUIRE( !w.terrain().player_terrain( ref_spanish ) );
  REQUIRE( !w.terrain().player_terrain( ref_dutch ) );

  REQUIRE( w.players().players[spanish]->type == spanish );
  REQUIRE( w.players().players[spanish]->nation ==
           e_nation::spanish );
  REQUIRE( w.players().players[ref_english]->type ==
           ref_english );
  REQUIRE( w.players().players[ref_english]->nation ==
           e_nation::english );
}

TEST_CASE( "[player-mgr] get_or_add_player" ) {
  world w;

  using enum e_player;

  REQUIRE( !w.players().players[english] );
  REQUIRE( !w.players().players[french] );
  REQUIRE( !w.players().players[spanish] );
  REQUIRE( !w.players().players[dutch] );
  REQUIRE( !w.players().players[ref_english] );
  REQUIRE( !w.players().players[ref_french] );
  REQUIRE( !w.players().players[ref_spanish] );
  REQUIRE( !w.players().players[ref_dutch] );

  REQUIRE( !w.terrain().player_terrain( english ) );
  REQUIRE( !w.terrain().player_terrain( french ) );
  REQUIRE( !w.terrain().player_terrain( spanish ) );
  REQUIRE( !w.terrain().player_terrain( dutch ) );
  REQUIRE( !w.terrain().player_terrain( ref_english ) );
  REQUIRE( !w.terrain().player_terrain( ref_french ) );
  REQUIRE( !w.terrain().player_terrain( ref_spanish ) );
  REQUIRE( !w.terrain().player_terrain( ref_dutch ) );

  get_or_add_player( w.ss(), w.ts(), spanish );

  REQUIRE( !w.players().players[english] );
  REQUIRE( !w.players().players[french] );
  REQUIRE( w.players().players[spanish] );
  REQUIRE( !w.players().players[dutch] );
  REQUIRE( !w.players().players[ref_english] );
  REQUIRE( !w.players().players[ref_french] );
  REQUIRE( !w.players().players[ref_spanish] );
  REQUIRE( !w.players().players[ref_dutch] );

  REQUIRE( !w.terrain().player_terrain( english ) );
  REQUIRE( !w.terrain().player_terrain( french ) );
  REQUIRE( w.terrain().player_terrain( spanish ) );
  REQUIRE( !w.terrain().player_terrain( dutch ) );
  REQUIRE( !w.terrain().player_terrain( ref_english ) );
  REQUIRE( !w.terrain().player_terrain( ref_french ) );
  REQUIRE( !w.terrain().player_terrain( ref_spanish ) );
  REQUIRE( !w.terrain().player_terrain( ref_dutch ) );

  REQUIRE( w.players().players[spanish]->type == spanish );
  REQUIRE( w.players().players[spanish]->nation ==
           e_nation::spanish );
  REQUIRE( w.players().players[spanish]->money == 0 );

  w.players().players[spanish]->money = 1;

  get_or_add_player( w.ss(), w.ts(), spanish );

  REQUIRE( !w.players().players[english] );
  REQUIRE( !w.players().players[french] );
  REQUIRE( w.players().players[spanish] );
  REQUIRE( !w.players().players[dutch] );
  REQUIRE( !w.players().players[ref_english] );
  REQUIRE( !w.players().players[ref_french] );
  REQUIRE( !w.players().players[ref_spanish] );
  REQUIRE( !w.players().players[ref_dutch] );

  REQUIRE( !w.terrain().player_terrain( english ) );
  REQUIRE( !w.terrain().player_terrain( french ) );
  REQUIRE( w.terrain().player_terrain( spanish ) );
  REQUIRE( !w.terrain().player_terrain( dutch ) );
  REQUIRE( !w.terrain().player_terrain( ref_english ) );
  REQUIRE( !w.terrain().player_terrain( ref_french ) );
  REQUIRE( !w.terrain().player_terrain( ref_spanish ) );
  REQUIRE( !w.terrain().player_terrain( ref_dutch ) );

  REQUIRE( w.players().players[spanish]->type == spanish );
  REQUIRE( w.players().players[spanish]->nation ==
           e_nation::spanish );

  // Thest that we haven't reset the old object.
  REQUIRE( w.players().players[spanish]->money == 1 );
}

} // namespace
} // namespace rn
