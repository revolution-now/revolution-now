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
TEST_CASE( "[trade-route] validate_token" ) {
  world w;
}

TEST_CASE( "[trade-route] sanitize_trade_routes" ) {
  world w;
}

TEST_CASE( "[trade-route] show_sanitization_actions" ) {
  world w;
}

TEST_CASE( "[trade-route] run_trade_route_sanitization" ) {
  world w;
}

TEST_CASE( "[trade-route] sanitize_unit_orders" ) {
  world w;
}

TEST_CASE( "[trade-route] look_up_trade_route (non-const)" ) {
  world w;
}

TEST_CASE( "[trade-route] look_up_trade_route (const)" ) {
  world w;
}

TEST_CASE( "[trade-route] look_up_trade_route_stop" ) {
  world w;
}

TEST_CASE( "[trade-route] look_up_next_trade_route_stop" ) {
  world w;
}

TEST_CASE( "[trade-route] curr_trade_route_target" ) {
  world w;
}

TEST_CASE( "[trade-route] are_all_stops_identical" ) {
  world w;
}

TEST_CASE(
    "[trade-route] convert_trade_route_target_to_goto_target" ) {
  world w;
}

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

TEST_CASE( "[trade-route] unit_has_reached_trade_route_stop" ) {
  world w;
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

TEST_CASE( "[trade-route] name_for_target" ) {
  world w;
}

TEST_CASE(
    "[trade-route] find_eligible_trade_routes_for_unit" ) {
  world w;
}

TEST_CASE( "[trade-route] select_trade_route" ) {
  world w;
}

TEST_CASE( "[trade-route] ask_first_stop" ) {
  world w;
}

TEST_CASE( "[trade-route] confirm_trade_route_orders" ) {
  world w;
}

} // namespace
} // namespace rn
