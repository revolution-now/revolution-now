/****************************************************************
**biomes-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-02-13.
*
* Description: Unit tests for the biomes module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/biomes.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/terrain-mgr.hpp"

// ss
#include "src/ss/ref.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    using enum e_player;
    add_player( english );
    set_default_player_type( english );
    set_default_player_as_human();
    create_default_map();
  }

  void create_default_map() {
    static MapSquare const S = make_ocean();
    static MapSquare const _ = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{/*
          0 1 2 3 4 5 6 7
      0*/ S,_,_,S,_,S,_,S, /*0
      1*/ S,_,S,_,S,_,S,S, /*1
      2*/ S,_,_,_,_,_,_,S, /*2
      3*/ S,_,_,_,_,_,_,S, /*3
      4*/ S,_,_,_,_,_,_,S, /*4
      5*/ S,_,_,_,_,_,_,S, /*5
      6*/ S,S,_,S,S,_,_,S, /*6
      7*/ S,_,S,S,_,S,_,S, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }

  void create_3x3_map() {
    static MapSquare const S = make_ocean();
    static MapSquare const _ = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{/*
          0 1 2
      0*/ S,_,_, /*0
      1*/ S,_,S, /*1
      2*/ S,_,_, /*2
          0 1 2
    */};
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[biomes] assign_arctic_biomes" ) {
  world w;

  auto const f = [&] [[clang::noinline]] {
    assign_arctic_biomes( w.rand(), w.real_terrain() );
  };

  auto const S = [&] [[clang::noinline]] (
                     int const x, int const y ) -> auto& {
    return w.square( { .x = x, .y = y } );
  };

  using enum e_biome;

  // Because e_biome{} == arctic this means that the water tiles
  // will have arctic as a biome by default which kind of inter-
  // feres with our testing here, so set those to prairie.
  on_all_tiles( w.ss(), [&]( point const p, MapSquare const& ) {
    if( w.square( p ).surface == e_surface::water )
      w.square( p ).ground = e_biome::prairie;
  } );

  REQUIRE( S( 0, 0 ).ground == prairie );
  REQUIRE( S( 1, 0 ).ground == grassland );
  REQUIRE( S( 2, 0 ).ground == grassland );
  REQUIRE( S( 3, 0 ).ground == prairie );
  REQUIRE( S( 4, 0 ).ground == grassland );
  REQUIRE( S( 5, 0 ).ground == prairie );
  REQUIRE( S( 6, 0 ).ground == grassland );
  REQUIRE( S( 7, 0 ).ground == prairie );

  REQUIRE( S( 0, 1 ).ground == prairie );
  REQUIRE( S( 1, 1 ).ground == grassland );
  REQUIRE( S( 2, 1 ).ground == prairie );
  REQUIRE( S( 3, 1 ).ground == grassland );
  REQUIRE( S( 4, 1 ).ground == prairie );
  REQUIRE( S( 5, 1 ).ground == grassland );
  REQUIRE( S( 6, 1 ).ground == prairie );
  REQUIRE( S( 7, 1 ).ground == prairie );

  REQUIRE( S( 0, 6 ).ground == prairie );
  REQUIRE( S( 1, 6 ).ground == prairie );
  REQUIRE( S( 2, 6 ).ground == grassland );
  REQUIRE( S( 3, 6 ).ground == prairie );
  REQUIRE( S( 4, 6 ).ground == prairie );
  REQUIRE( S( 5, 6 ).ground == grassland );
  REQUIRE( S( 6, 6 ).ground == grassland );
  REQUIRE( S( 7, 6 ).ground == prairie );

  REQUIRE( S( 0, 7 ).ground == prairie );
  REQUIRE( S( 1, 7 ).ground == grassland );
  REQUIRE( S( 2, 7 ).ground == prairie );
  REQUIRE( S( 3, 7 ).ground == prairie );
  REQUIRE( S( 4, 7 ).ground == grassland );
  REQUIRE( S( 5, 7 ).ground == prairie );
  REQUIRE( S( 6, 7 ).ground == grassland );
  REQUIRE( S( 7, 7 ).ground == prairie );

  w.rand().EXPECT__bernoulli( .5 ).returns( false );
  w.rand().EXPECT__bernoulli( .5 ).returns( true );
  w.rand().EXPECT__bernoulli( .5 ).returns( false );
  w.rand().EXPECT__bernoulli( .5 ).returns( true );
  w.rand().EXPECT__bernoulli( .5 ).returns( false );
  w.rand().EXPECT__bernoulli( .5 ).returns( true );
  f();

  REQUIRE( S( 0, 0 ).ground == prairie );
  REQUIRE( S( 1, 0 ).ground == arctic );
  REQUIRE( S( 2, 0 ).ground == arctic );
  REQUIRE( S( 3, 0 ).ground == prairie );
  REQUIRE( S( 4, 0 ).ground == arctic );
  REQUIRE( S( 5, 0 ).ground == prairie );
  REQUIRE( S( 6, 0 ).ground == arctic );
  REQUIRE( S( 7, 0 ).ground == prairie );

  REQUIRE( S( 0, 1 ).ground == prairie );
  REQUIRE( S( 1, 1 ).ground == grassland );
  REQUIRE( S( 2, 1 ).ground == prairie );
  REQUIRE( S( 3, 1 ).ground == arctic );
  REQUIRE( S( 4, 1 ).ground == prairie );
  REQUIRE( S( 5, 1 ).ground == grassland );
  REQUIRE( S( 6, 1 ).ground == prairie );
  REQUIRE( S( 7, 1 ).ground == prairie );

  REQUIRE( S( 0, 6 ).ground == prairie );
  REQUIRE( S( 1, 6 ).ground == prairie );
  REQUIRE( S( 2, 6 ).ground == arctic );
  REQUIRE( S( 3, 6 ).ground == prairie );
  REQUIRE( S( 4, 6 ).ground == prairie );
  REQUIRE( S( 5, 6 ).ground == grassland );
  REQUIRE( S( 6, 6 ).ground == arctic );
  REQUIRE( S( 7, 6 ).ground == prairie );

  REQUIRE( S( 0, 7 ).ground == prairie );
  REQUIRE( S( 1, 7 ).ground == arctic );
  REQUIRE( S( 2, 7 ).ground == prairie );
  REQUIRE( S( 3, 7 ).ground == prairie );
  REQUIRE( S( 4, 7 ).ground == arctic );
  REQUIRE( S( 5, 7 ).ground == prairie );
  REQUIRE( S( 6, 7 ).ground == arctic );
  REQUIRE( S( 7, 7 ).ground == prairie );

  w.create_3x3_map();
  on_all_tiles( w.ss(), [&]( point const p, MapSquare const& ) {
    if( w.square( p ).surface == e_surface::water )
      w.square( p ).ground = e_biome::prairie;
  } );

  REQUIRE( S( 0, 0 ).ground == prairie );
  REQUIRE( S( 1, 0 ).ground == grassland );
  REQUIRE( S( 2, 0 ).ground == grassland );
  REQUIRE( S( 0, 1 ).ground == prairie );
  REQUIRE( S( 1, 1 ).ground == grassland );
  REQUIRE( S( 2, 1 ).ground == prairie );
  REQUIRE( S( 0, 2 ).ground == prairie );
  REQUIRE( S( 1, 2 ).ground == grassland );
  REQUIRE( S( 2, 2 ).ground == grassland );

  f();

  REQUIRE( S( 0, 0 ).ground == prairie );
  REQUIRE( S( 1, 0 ).ground == grassland );
  REQUIRE( S( 2, 0 ).ground == grassland );
  REQUIRE( S( 0, 1 ).ground == prairie );
  REQUIRE( S( 1, 1 ).ground == grassland );
  REQUIRE( S( 2, 1 ).ground == prairie );
  REQUIRE( S( 0, 2 ).ground == prairie );
  REQUIRE( S( 1, 2 ).ground == grassland );
  REQUIRE( S( 2, 2 ).ground == grassland );
}

TEST_CASE( "[biomes] assign_biomes" ) {
  world w;
}

} // namespace
} // namespace rn
