/****************************************************************
**ref-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-18.
*
* Description: Unit tests for the ref module.
*
*****************************************************************/
#include "revolution.rds.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/ref.hpp"

// Testing.
#include "test/fake/world.hpp"

// refl
#include "src/refl/to-str.hpp"

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
    add_default_player();
    create_default_map();
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
TEST_CASE( "[ref] evolved_royal_money" ) {
  world w;
}

TEST_CASE( "[ref] apply_royal_money_change" ) {
  world w;
}

TEST_CASE( "[ref] select_next_ref_type" ) {
  world w;
}

TEST_CASE( "[ref] add_ref_unit" ) {
  world w;
}

TEST_CASE( "[ref] ref_unit_to_unit_type" ) {
  world w;
}

TEST_CASE( "[ref] add_ref_unit_ui_seq" ) {
  world w;
}

TEST_CASE( "[ref] add_ref_unit (loop)" ) {
  world w;

  ExpeditionaryForce force, expected;

  auto const one_round = [&] {
    e_expeditionary_force_type const type =
        select_next_ref_type( force );
    add_ref_unit( force, type );
  };

  expected = {
    .regular   = 0,
    .cavalry   = 0,
    .artillery = 0,
    .man_o_war = 0,
  };
  REQUIRE( force == expected );

  for( int i = 0; i < 1000; ++i ) one_round();

  expected = {
    .regular   = 580,
    .cavalry   = 190,
    .artillery = 140,
    .man_o_war = 90,
  };
  REQUIRE( force == expected );
}

} // namespace
} // namespace rn
