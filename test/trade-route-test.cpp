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

// Revolution Now
#include "src/colony-mgr.hpp"

// ss
#include "src/ss/colonies.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// C++ standard library
#include <ranges>

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;
using ::std::ranges::views::zip;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    using enum e_player;
    add_player( english );
    set_default_player_type( english );
    set_default_player_type( english );
    set_default_player_as_human();
    create_default_map();
  }

  void create_default_map() {
    static MapSquare const S = make_ocean();
    static MapSquare const _ = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7
      0*/ S,_,_,_,_,_,_,S, /*0
      1*/ S,_,_,_,_,_,_,S, /*1
      2*/ S,_,_,_,_,_,_,S, /*2
      3*/ S,_,_,_,_,_,_,S, /*3
      4*/ S,_,_,_,_,_,_,S, /*4
      5*/ S,_,_,_,_,_,_,S, /*5
      6*/ S,_,_,_,_,_,_,S, /*6
      7*/ S,_,_,_,_,_,_,S, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }

  static int constexpr kNumColonies = 6;

  void create_colonies() {
    //      0 1 2 3 4 5 6 7
    //  -----------------------
    //  0|  S c _ _ _ _ _ S  |0
    //  1|  S _ _ _ _ _ c S  |1
    //  2|  S _ _ _ _ _ _ S  |2
    //  3|  S _ _ c _ _ _ S  |3
    //  4|  S c _ _ _ _ _ S  |4
    //  5|  S _ _ c _ _ _ S  |5
    //  6|  S _ _ _ _ _ _ S  |6
    //  7|  S _ _ _ _ _ c S  |7
    //  -----------------------
    //      0 1 2 3 4 5 6 7
    static array<point, kNumColonies> const kColonyTiles = {
      point{ .x = 1, .y = 0 }, //
      point{ .x = 6, .y = 1 }, //
      point{ .x = 3, .y = 3 }, //
      point{ .x = 1, .y = 4 }, //
      point{ .x = 3, .y = 5 }, //
      point{ .x = 6, .y = 7 }, //
    };
    if( !colonies().all().empty() ) return;
    for( auto const [tile, p_colony] :
         zip( kColonyTiles, colonies_ ) )
      p_colony = &add_colony( tile );
  }

  void add_route( TradeRoute&& route ) {
    auto& tr              = trade_routes();
    TradeRouteId const id = ++tr.prev_trade_route_id;

    route.id      = id;
    route.name    = format( "{}.{}", route.type, id );
    route.player  = default_player_type();
    tr.routes[id] = std::move( route );
  }

  // [1] <-> [2], exchanging furs/coats.
  void add_land_route_1() {
    using enum e_commodity;
    using enum e_trade_route_type;
    create_colonies();
    add_route( TradeRoute{
      .type  = land,
      .stops = {
        { .target =
              TradeRouteTarget::colony{
                .colony_id = get<0>( colonies_ )->id,
              },
          .loads   = { furs },
          .unloads = { coats } },
        { .target =
              TradeRouteTarget::colony{
                .colony_id = get<1>( colonies_ )->id,
              },
          .loads   = { coats },
          .unloads = { furs } },
      } } );
  }

 public:
  array<Colony*, kNumColonies> colonies_;
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_WORLD(
    "[trade-route] sanitize_trade_routes / validate_token" ) {
  Player& player = default_player();
  vector<TradeRouteSanitizationAction> actions_taken, expected;
  using A = TradeRouteSanitizationAction;
  using enum e_player;

  auto const f = [&] [[clang::noinline]] {
    actions_taken.clear();
    TradeRoutesSanitizedToken const& token =
        sanitize_trade_routes( ss(), as_const( player ),
                               trade_routes(), actions_taken );
    validate_token( token );
  };

  // Default.
  f();
  expected = {};
  REQUIRE( actions_taken == expected );

  SECTION( "too many stops" ) {
    add_land_route_1();
    add_land_route_1();
    auto old = trade_routes();
    trade_routes().routes.at( 2 ).stops.resize( 6 );
    f();
    expected = { A::too_many_stops{ .route_id   = 2,
                                    .route_name = "land.2" } };
    REQUIRE( actions_taken == expected );
    old.routes.at( 2 ).stops.resize( 4 );
    REQUIRE( trade_routes() == old );
  }

  SECTION( "too many stops wrong player" ) {
    add_land_route_1();
    add_land_route_1();
    trade_routes().routes.at( 2 ).player = french;
    trade_routes().routes.at( 2 ).stops.resize( 6 );
    auto old = trade_routes();
    f();
    expected = {};
    REQUIRE( actions_taken == expected );
    REQUIRE( trade_routes() == old );
  }

  SECTION( "empty trade routes" ) {
    add_land_route_1();
    add_land_route_1();
    add_land_route_1();
    trade_routes().routes.at( 2 ).stops.clear();
    auto old = trade_routes();
    f();
    expected = { A::empty_route{ .route_name = "land.2" } };
    REQUIRE( actions_taken == expected );
    old.routes.erase( 2 );
    REQUIRE( trade_routes() == old );
  }

  SECTION( "harbor inaccessible" ) {
    add_land_route_1();
    add_land_route_1();
    add_land_route_1();
    auto old = trade_routes();
    trade_routes().routes.at( 2 ).stops.insert(
        trade_routes().routes.at( 2 ).stops.begin(),
        TradeRouteStop{ .target = TradeRouteTarget::harbor{} } );
    trade_routes().routes.at( 2 ).stops.push_back(
        TradeRouteStop{ .target = TradeRouteTarget::harbor{} } );
    player.revolution.status = e_revolution_status::declared;
    f();
    expected = {
      A::no_harbor_post_declaration{ .route_name = "land.2" },
      A::no_harbor_post_declaration{ .route_name = "land.2" } };
    REQUIRE( actions_taken == expected );
    REQUIRE( trade_routes() == old );
  }

  SECTION( "one colony changed player" ) {
    add_land_route_1();
    auto old = trade_routes();

    get<0>( colonies_ )->player = french;
    f();
    expected = { A::colony_changed_player{
      .colony_id = 1, .route_name = "land.1" } };
    REQUIRE( actions_taken == expected );
    old.routes.at( 1 ).stops.erase(
        old.routes.at( 1 ).stops.begin() );
    REQUIRE( trade_routes() == old );
  }

  SECTION( "both colonies changed player" ) {
    add_land_route_1();
    auto old = trade_routes();

    get<0>( colonies_ )->player = french;
    get<1>( colonies_ )->player = french;
    f();
    expected = { A::colony_changed_player{
                   .colony_id = 1, .route_name = "land.1" },
                 A::colony_changed_player{
                   .colony_id = 2, .route_name = "land.1" },
                 A::empty_route{ .route_name = "land.1" } };
    REQUIRE( actions_taken == expected );
    old.routes.erase( 1 );
    REQUIRE( trade_routes() == old );
  }

  SECTION( "colony no longer exists" ) {
    add_land_route_1();
    auto old = trade_routes();
    destroy_colony( ss(), ts(), *get<0>( colonies_ ) );
    f();
    expected = { A::colony_no_longer_exists{
      .route_id = 1, .route_name = "land.1" } };
    REQUIRE( actions_taken == expected );
    old.routes.at( 1 ).stops.erase(
        old.routes.at( 1 ).stops.begin() );
    REQUIRE( trade_routes() == old );
  }

  SECTION( "all colonies gone" ) {
    add_land_route_1();
    auto old = trade_routes();
    kill_all_colonies();
    f();
    expected = { A::colony_no_longer_exists{
                   .route_id = 1, .route_name = "land.1" },
                 A::colony_no_longer_exists{
                   .route_id = 1, .route_name = "land.1" },
                 A::empty_route{ .route_name = "land.1" } };
    REQUIRE( actions_taken == expected );
    old.routes.erase( 1 );
    REQUIRE( trade_routes() == old );
  }
}

TEST_CASE( "[trade-route] show_sanitization_actions" ) {
  world w;
}

TEST_CASE( "[trade-route] run_trade_route_sanitization" ) {
  world w;
}

TEST_CASE( "[trade-route] sanitize_unit_trade_route_orders" ) {
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

TEST_CASE( "[trade-route] trade_route_unload" ) {
  world w;
}

TEST_CASE( "[trade-route] colony_commodities_by_value (all)" ) {
  world w;
}

TEST_CASE(
    "[trade-route] colony_commodities_by_value (restricted)" ) {
  world w;
}

TEST_CASE( "[trade-route] trade_route_load" ) {
  world w;
}

} // namespace
} // namespace rn
