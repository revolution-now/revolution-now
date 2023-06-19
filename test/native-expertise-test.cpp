/****************************************************************
**native-expertise.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-12.
*
* Description: Unit tests for the src/native-expertise.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/native-expertise.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/irand.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/ref.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

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
    add_player( e_nation::english );
    set_default_player( e_nation::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, L, L, //
        L, L, L, L, L, //
        _, L, L, L, L, //
        _, L, L, L, L, //
        _, _, L, _, L, //
    };
    tiles[0]  = make_terrain( e_terrain::desert );
    tiles[1]  = make_terrain( e_terrain::scrub );
    tiles[2]  = make_terrain( e_terrain::grassland );
    tiles[3]  = make_terrain( e_terrain::conifer );
    tiles[4]  = make_terrain( e_terrain::marsh );
    tiles[5]  = make_terrain( e_terrain::wetland );
    tiles[6]  = make_terrain( e_terrain::plains );
    tiles[7]  = make_terrain( e_terrain::mixed );
    tiles[8]  = make_terrain( e_terrain::prairie );
    tiles[9]  = make_terrain( e_terrain::broadleaf );
    tiles[10] = make_terrain( e_terrain::savannah );
    tiles[11] = make_terrain( e_terrain::tropical );
    tiles[12] = make_terrain( e_terrain::swamp );
    tiles[13] = make_terrain( e_terrain::rain );
    tiles[14] = make_terrain( e_terrain::tundra );
    tiles[15] = make_terrain( e_terrain::boreal );
    tiles[16] = make_terrain( e_terrain::arctic );
    tiles[17] = make_terrain( e_terrain::hills );
    tiles[18] = make_terrain( e_terrain::mountains );

    build_map( std::move( tiles ), 5 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[native-expertise] select_expertise_for_dwelling" ) {
  World W;

  refl::enum_map<e_native_skill, int> const weights{
      { e_native_skill::farming, 10 },
      { e_native_skill::fishing, 9 },
      { e_native_skill::sugar_planting, 8 },
      { e_native_skill::tobacco_planting, 7 },
      { e_native_skill::cotton_planting, 6 },
      { e_native_skill::fur_trapping, 5 },
      { e_native_skill::ore_mining, 4 },
      { e_native_skill::silver_mining, 3 },
      { e_native_skill::fur_trading, 2 },
      { e_native_skill::scouting, 1 },
  };

  W.rand()
      .EXPECT__between_ints( 0, 55, e_interval::half_open )
      .returns( 10 );
  e_native_skill const res =
      select_expertise_for_dwelling( W.ts(), weights );
  REQUIRE( res == e_native_skill::fishing );
}

TEST_CASE( "[native-expertise] dwelling_expertise_weights" ) {
  World                               W;
  refl::enum_map<e_native_skill, int> expected;
  Dwelling*                           dwelling = nullptr;

  auto f = [&] {
    return dwelling_expertise_weights( W.ss(), *dwelling );
  };

  SECTION( "semi-nomadic" ) {
    dwelling =
        &W.add_dwelling( { .x = 2, .y = 2 }, e_tribe::tupi );
    expected = { { { e_native_skill::farming, 47 },
                   { e_native_skill::fishing, 8 },
                   { e_native_skill::sugar_planting, 420 },
                   { e_native_skill::tobacco_planting, 165 },
                   { e_native_skill::cotton_planting, 80 },
                   { e_native_skill::fur_trapping, 170 },
                   { e_native_skill::ore_mining, 0 },
                   { e_native_skill::silver_mining, 0 },
                   { e_native_skill::fur_trading, 0 },
                   { e_native_skill::scouting, 120 } } };
    REQUIRE( f() == expected );
  }

  SECTION( "agrarian" ) {
    dwelling =
        &W.add_dwelling( { .x = 2, .y = 2 }, e_tribe::cherokee );
    expected = { { { e_native_skill::farming, 141 },
                   { e_native_skill::fishing, 48 },
                   { e_native_skill::sugar_planting, 140 },
                   { e_native_skill::tobacco_planting, 66 },
                   { e_native_skill::cotton_planting, 32 },
                   { e_native_skill::fur_trapping, 119 },
                   { e_native_skill::ore_mining, 136 },
                   { e_native_skill::silver_mining, 0 },
                   { e_native_skill::fur_trading, 14 },
                   { e_native_skill::scouting, 52 } } };
    REQUIRE( f() == expected );
  }

  SECTION( "civilized" ) {
    dwelling =
        &W.add_dwelling( { .x = 2, .y = 2 }, e_tribe::inca );
    expected = { { { e_native_skill::farming, 2350 },
                   { e_native_skill::fishing, 520 },
                   { e_native_skill::sugar_planting, 21 },
                   { e_native_skill::tobacco_planting, 0 },
                   { e_native_skill::cotton_planting, 0 },
                   { e_native_skill::fur_trapping, 17 },
                   { e_native_skill::ore_mining, 136 },
                   { e_native_skill::silver_mining, 0 },
                   { e_native_skill::fur_trading, 20 },
                   { e_native_skill::scouting, 4 } } };
    REQUIRE( f() == expected );
  }
}

} // namespace
} // namespace rn
