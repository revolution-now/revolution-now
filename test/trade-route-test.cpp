/****************************************************************
**trade-route-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Unit tests for the trade-route module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/trade-route.hpp"

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
struct world : testing::World {
  world() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    static MapSquare const _ = make_ocean();
    static MapSquare const X = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7
      0*/ _,X,X,X,X,X,X,_, /*0
      1*/ _,X,X,X,X,X,X,_, /*1
      2*/ _,X,X,X,X,X,X,_, /*2
      3*/ _,X,X,X,X,X,X,_, /*3
      4*/ _,X,X,X,X,X,X,_, /*4
      5*/ _,X,X,X,X,X,X,_, /*5
      6*/ _,X,X,X,X,X,X,_, /*6
      7*/ _,X,X,X,X,X,X,_, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[trade-route] unit_can_start_trade_route" ) {
  world w;

  using enum e_unit_type;

  auto const f =
      [&] [[clang::noinline]] ( e_unit_type const type ) {
        return unit_can_start_trade_route( type );
      };

  REQUIRE( f( caravel ) );
  REQUIRE( f( privateer ) );
  REQUIRE( f( galleon ) );
  REQUIRE( f( man_o_war ) );
  REQUIRE( f( wagon_train ) );

  REQUIRE_FALSE( f( free_colonist ) );
  REQUIRE_FALSE( f( scout ) );
  REQUIRE_FALSE( f( dragoon ) );
  REQUIRE_FALSE( f( cavalry ) );
  REQUIRE_FALSE( f( artillery ) );
  REQUIRE_FALSE( f( treasure ) );
}

TEST_CASE( "[trade-route] ask_edit_trade_route" ) {
  world w;
}

TEST_CASE( "[trade-route] ask_delete_trade_route" ) {
  world w;
}

TEST_CASE( "[trade-route] ask_create_trade_route" ) {
  world w;
}

TEST_CASE( "[trade-route] create_trade_route_object" ) {
  world w;
}

TEST_CASE( "[trade-route] confirm_delete_trade_route" ) {
  world w;
}

TEST_CASE( "[trade-route] delete_trade_route" ) {
  world w;
}

TEST_CASE( "[trade-route] available_colonies_for_route" ) {
  world w;
}

} // namespace
} // namespace rn
