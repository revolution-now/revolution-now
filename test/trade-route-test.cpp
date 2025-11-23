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
#include "test/mocking.hpp"
#include "test/mocks/iagent.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/colony-mgr.hpp"

// ss
#include "src/ss/colonies.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/scope-exit.hpp"
#include "src/base/to-str-ext-std.hpp"

// C++ standard library
#include <ranges>

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;
using ::mock::matchers::StrContains;
using ::std::ranges::views::zip;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    using enum e_player;
    add_player( english );
    add_player( french );
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
    TradeRouteId const id = ++tr.last_trade_route_id;

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

  // [2] <-> [2], exchanging furs/coats.
  void add_land_route_2() {
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
                .colony_id = get<0>( colonies_ )->id,
              },
          .loads   = { coats },
          .unloads = { furs } },
      } } );
  }

  // [1] --> [harbor] --> [2].
  void add_sea_route_1() {
    using enum e_commodity;
    using enum e_trade_route_type;
    create_colonies();
    add_route( TradeRoute{
      .type  = sea,
      .stops = {
        { .target =
              TradeRouteTarget::colony{
                .colony_id = get<0>( colonies_ )->id,
              },
          .loads   = { furs },
          .unloads = { coats } },
        { .target  = TradeRouteTarget::harbor{},
          .loads   = { trade_goods },
          .unloads = { furs } },
        { .target =
              TradeRouteTarget::colony{
                .colony_id = get<1>( colonies_ )->id,
              },
          .loads   = { coats },
          .unloads = { trade_goods } },
      } } );
  }

  // [harbor] --> [harbor].
  void add_double_harbor_route() {
    using enum e_commodity;
    using enum e_trade_route_type;
    create_colonies();
    add_route(
        TradeRoute{ .type  = sea,
                    .stops = {
                      { .target  = TradeRouteTarget::harbor{},
                        .loads   = { trade_goods },
                        .unloads = { furs } },
                      { .target  = TradeRouteTarget::harbor{},
                        .loads   = { furs },
                        .unloads = { trade_goods } },
                    } } );
  }

  TradeRoutesSanitizedToken const& token() {
    vector<TradeRouteSanitizationAction> actions_taken;
    SCOPE_EXIT { CHECK( actions_taken.empty() ); };
    return sanitize_trade_routes(
        ss(), default_player(), trade_routes(), actions_taken );
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
    expected = { A::too_many_stops{ .route_name = "land.2" } };
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
    expected = {
      A::colony_no_longer_exists{ .route_name = "land.1" } };
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
    expected = {
      A::colony_no_longer_exists{ .route_name = "land.1" },
      A::colony_no_longer_exists{ .route_name = "land.1" },
      A::empty_route{ .route_name = "land.1" } };
    REQUIRE( actions_taken == expected );
    old.routes.erase( 1 );
    REQUIRE( trade_routes() == old );
  }
}

TEST_CASE( "[trade-route] show_sanitization_actions" ) {
  world w;
  auto const& token = w.token();
  vector<TradeRouteSanitizationAction> actions_taken;
  using A = TradeRouteSanitizationAction;

  auto const f = [&] [[clang::noinline]] {
    co_await_test( show_sanitization_actions(
        w.ss(), w.gui(), w.agent(), actions_taken, token ) );
  };

  // Default.
  f();

  // colony_no_longer_exists
  actions_taken = {
    A::colony_no_longer_exists{ .route_name = "some route" } };
  w.agent().EXPECT__message_box( StrContains(
      "[some route] trade route has been removed from the "
      "itinerary as it no longer exists" ) );
  f();

  w.add_colony( { .x = 1, .y = 0 }, e_player::spanish ).name =
      "some colony";

  // colony_changed_player
  // no_harbor_post_declaration
  actions_taken = {
    A::colony_changed_player{ .colony_id  = 1,
                              .route_name = "some route" },
    A::no_harbor_post_declaration{ .route_name =
                                       "some route" } };
  w.agent().EXPECT__message_box( StrContains(
      "The colony of [some colony], listed as a stop on the "
      "[some route] trade route, has been removed from the "
      "itinerary as it is now occupied by the [Spanish]" ) );
  w.agent().EXPECT__human().returns( true );
  w.gui().EXPECT__wait_for( chrono::milliseconds{ 200 } );
  w.agent().EXPECT__message_box(
      StrContains( "All European Harbor stops on the [some "
                   "route] trade route have been removed" ) );
  f();

  // empty_route
  actions_taken = {
    A::empty_route{ .route_name = "some route" } };
  w.agent().EXPECT__message_box(
      StrContains( "The [some route] trade route has been left "
                   "with no stops" ) );
  f();

  // too_many_stops
  actions_taken = {
    A::too_many_stops{ .route_name = "some route" } };
  w.agent().EXPECT__message_box(
      StrContains( "The [some route] trade route has exceeded "
                   "the limit of four stops" ) );
  f();

  // colony_changed_player
  // no_harbor_post_declaration
  // empty_route
  actions_taken = {
    A::colony_changed_player{ .colony_id  = 1,
                              .route_name = "some route" },
    A::no_harbor_post_declaration{ .route_name = "some route" },
    A::empty_route{ .route_name = "some route" } };
  w.agent().EXPECT__message_box( StrContains(
      "The colony of [some colony], listed as a stop on the "
      "[some route] trade route, has been removed from the "
      "itinerary as it is now occupied by the [Spanish]" ) );
  w.agent().EXPECT__human().returns( true );
  w.gui().EXPECT__wait_for( chrono::milliseconds{ 200 } );
  w.agent().EXPECT__message_box(
      StrContains( "All European Harbor stops on the [some "
                   "route] trade route have been removed" ) );
  w.agent().EXPECT__human().returns( true );
  w.gui().EXPECT__wait_for( chrono::milliseconds{ 200 } );
  w.agent().EXPECT__message_box(
      StrContains( "The [some route] trade route has been left "
                   "with no stops" ) );
  f();

  // (non-human)
  // colony_changed_player
  // no_harbor_post_declaration
  // empty_route
  actions_taken = {
    A::colony_changed_player{ .colony_id  = 1,
                              .route_name = "some route" },
    A::no_harbor_post_declaration{ .route_name = "some route" },
    A::empty_route{ .route_name = "some route" } };
  w.agent().EXPECT__message_box( StrContains(
      "The colony of [some colony], listed as a stop on the "
      "[some route] trade route, has been removed from the "
      "itinerary as it is now occupied by the [Spanish]" ) );
  w.agent().EXPECT__human().returns( false );
  w.agent().EXPECT__message_box(
      StrContains( "All European Harbor stops on the [some "
                   "route] trade route have been removed" ) );
  w.agent().EXPECT__human().returns( false );
  w.agent().EXPECT__message_box(
      StrContains( "The [some route] trade route has been left "
                   "with no stops" ) );
  f();
}

// This one is abbreviated because it is just composed of two
// function calls that are already tested.
TEST_WORLD( "[trade-route] run_trade_route_sanitization" ) {
  Player& player = default_player();
  using enum e_player;

  auto const f = [&] [[clang::noinline]]
      -> TradeRoutesSanitizedToken const& {
    auto const token =
        co_await_test( run_trade_route_sanitization(
            ss(), as_const( player ), gui(), trade_routes(),
            agent() ) );
    validate_token( token );
    return token;
  };

  add_land_route_1();
  add_land_route_1();
  auto old = trade_routes();
  trade_routes().routes.at( 2 ).stops.resize( 6 );
  agent().EXPECT__message_box(
      StrContains( "The [land.2] trade route has exceeded the "
                   "limit of four stops" ) );
  f();
  old.routes.at( 2 ).stops.resize( 4 );
  REQUIRE( trade_routes() == old );
}

TEST_CASE( "[trade-route] sanitize_unit_trade_route_orders" ) {
  world w;
  w.add_land_route_1();
  w.add_land_route_1();
  w.add_land_route_1();
  // Remove the second one.
  w.trade_routes().routes.erase( 2 );
  auto const& token = w.token();

  Unit& unit = w.add_unit_on_map( e_unit_type::wagon_train,
                                  { .x = 1, .y = 2 } );

  auto const f = [&] [[clang::noinline]] {
    return sanitize_unit_trade_route_orders( w.ss(), unit,
                                             token );
  };

  auto& orders =
      unit.orders().emplace<unit_orders::trade_route>();

  orders.id               = 1;
  orders.en_route_to_stop = 1;
  REQUIRE( f() == unit_orders::trade_route{
                    .id = 1, .en_route_to_stop = 1 } );

  SECTION( "wrong orders" ) {
    unit.clear_orders();
    REQUIRE( f() == nothing );
  }

  SECTION( "non-existent route" ) {
    orders.id = 2;
    REQUIRE( f() == nothing );
  }

  SECTION( "wrong player" ) {
    w.trade_routes().routes.at( 1 ).player = e_player::spanish;
    REQUIRE( f() == nothing );
  }

  SECTION( "empty stops" ) {
    w.trade_routes().routes.at( 1 ).stops.clear();
    REQUIRE( f() == nothing );
  }

  SECTION( "next stop doesn't exist" ) {
    w.trade_routes().routes.at( 1 ).stops.resize( 1 );
    REQUIRE( orders.en_route_to_stop == 1 ); // sanity check.
    REQUIRE( f() == unit_orders::trade_route{
                      .id = 1, .en_route_to_stop = 0 } );
  }
}

TEST_CASE( "[trade-route] look_up_trade_route (non-const)" ) {
  world w;
  w.add_land_route_1();
  w.add_land_route_1();
  w.add_land_route_1();
  w.add_land_route_1();

  w.trade_routes().routes.erase( 3 );

  auto const f =
      [&] [[clang::noinline]] ( TradeRouteId const id ) {
        return look_up_trade_route( w.trade_routes(), id );
      };
  static_assert(
      is_same_v<decltype( f( 1 ) ), maybe<TradeRoute&>> );

  REQUIRE( &f( 1 ).value() == &w.trade_routes().routes.at( 1 ) );
  REQUIRE( &f( 2 ).value() == &w.trade_routes().routes.at( 2 ) );
  REQUIRE( f( 3 ) == nothing );
  REQUIRE( &f( 4 ).value() == &w.trade_routes().routes.at( 4 ) );
}

TEST_CASE( "[trade-route] look_up_trade_route (const)" ) {
  world w;
  w.add_land_route_1();
  w.add_land_route_1();
  w.add_land_route_1();
  w.add_land_route_1();

  w.trade_routes().routes.erase( 3 );

  auto const f =
      [&] [[clang::noinline]] ( TradeRouteId const id ) {
        return look_up_trade_route( as_const( w.trade_routes() ),
                                    id );
      };
  static_assert(
      is_same_v<decltype( f( 1 ) ), maybe<TradeRoute const&>> );

  REQUIRE( &f( 1 ).value() == &w.trade_routes().routes.at( 1 ) );
  REQUIRE( &f( 2 ).value() == &w.trade_routes().routes.at( 2 ) );
  REQUIRE( f( 3 ) == nothing );
  REQUIRE( &f( 4 ).value() == &w.trade_routes().routes.at( 4 ) );
}

TEST_CASE( "[trade-route] look_up_trade_route_stop" ) {
  world w;
  TradeRoute route;

  auto const f = [&] [[clang::noinline]] ( int const stop ) {
    return look_up_trade_route_stop( as_const( route ), stop );
  };

  REQUIRE( f( 0 ) == nothing );
  REQUIRE( f( 1 ) == nothing );
  REQUIRE( f( 2 ) == nothing );

  route.stops.resize( 1 );
  REQUIRE( &f( 0 ).value() == &route.stops.at( 0 ) );
  REQUIRE( f( 1 ) == nothing );
  REQUIRE( f( 2 ) == nothing );

  route.stops.resize( 2 );
  REQUIRE( &f( 0 ).value() == &route.stops.at( 0 ) );
  REQUIRE( &f( 1 ).value() == &route.stops.at( 1 ) );
  REQUIRE( f( 2 ) == nothing );

  route.stops.resize( 3 );
  REQUIRE( &f( 0 ).value() == &route.stops.at( 0 ) );
  REQUIRE( &f( 1 ).value() == &route.stops.at( 1 ) );
  REQUIRE( &f( 2 ).value() == &route.stops.at( 2 ) );
}

TEST_CASE( "[trade-route] look_up_next_trade_route_stop" ) {
  world w;
  TradeRoute route;

  auto const f = [&] [[clang::noinline]] ( int const stop ) {
    return look_up_next_trade_route_stop( as_const( route ),
                                          stop );
  };

  REQUIRE( f( 0 ) == nothing );
  REQUIRE( f( 1 ) == nothing );
  REQUIRE( f( 2 ) == nothing );
  REQUIRE( f( 3 ) == nothing );
  REQUIRE( f( 4 ) == nothing );

  route.stops.resize( 1 );
  REQUIRE( &f( 0 ).value() == &route.stops.at( 0 ) );
  REQUIRE( &f( 1 ).value() == &route.stops.at( 0 ) );
  REQUIRE( &f( 2 ).value() == &route.stops.at( 0 ) );
  REQUIRE( &f( 3 ).value() == &route.stops.at( 0 ) );
  REQUIRE( &f( 4 ).value() == &route.stops.at( 0 ) );

  route.stops.resize( 2 );
  REQUIRE( &f( 0 ).value() == &route.stops.at( 1 ) );
  REQUIRE( &f( 1 ).value() == &route.stops.at( 0 ) );
  REQUIRE( &f( 2 ).value() == &route.stops.at( 0 ) );
  REQUIRE( &f( 3 ).value() == &route.stops.at( 0 ) );
  REQUIRE( &f( 4 ).value() == &route.stops.at( 0 ) );

  route.stops.resize( 3 );
  REQUIRE( &f( 0 ).value() == &route.stops.at( 1 ) );
  REQUIRE( &f( 1 ).value() == &route.stops.at( 2 ) );
  REQUIRE( &f( 2 ).value() == &route.stops.at( 0 ) );
  REQUIRE( &f( 3 ).value() == &route.stops.at( 0 ) );
  REQUIRE( &f( 4 ).value() == &route.stops.at( 0 ) );

  route.stops.resize( 4 );
  REQUIRE( &f( 0 ).value() == &route.stops.at( 1 ) );
  REQUIRE( &f( 1 ).value() == &route.stops.at( 2 ) );
  REQUIRE( &f( 2 ).value() == &route.stops.at( 3 ) );
  REQUIRE( &f( 3 ).value() == &route.stops.at( 0 ) );
  REQUIRE( &f( 4 ).value() == &route.stops.at( 0 ) );
}

TEST_CASE( "[trade-route] curr_trade_route_target" ) {
  world w;
  w.add_land_route_1();
  w.add_land_route_1();
  w.add_land_route_1();

  Unit& unit = w.add_unit_on_map( e_unit_type::wagon_train,
                                  { .x = 1, .y = 2 } );

  auto const f = [&] [[clang::noinline]] {
    return curr_trade_route_target( w.trade_routes(), unit );
  };

  unit.orders() =
      unit_orders::trade_route{ .id = 2, .en_route_to_stop = 1 };
  REQUIRE( f() == TradeRouteTarget::colony{
                    .colony_id = w.colonies_[1]->id } );

  SECTION( "bad stop" ) {
    unit.orders()
        .get_if<unit_orders::trade_route>()
        .value()
        .en_route_to_stop = 2;
    REQUIRE( f() == nothing );
  }

  SECTION( "bad route" ) {
    unit.orders().get_if<unit_orders::trade_route>().value().id =
        4;
    REQUIRE( f() == nothing );
  }

  SECTION( "bad orders" ) {
    unit.orders() = unit_orders::go_to{};
    REQUIRE( f() == nothing );
    unit.clear_orders();
    REQUIRE( f() == nothing );
  }
}

TEST_CASE( "[trade-route] are_all_stops_identical" ) {
  world w;

  auto const f =
      [&] [[clang::noinline]] ( TradeRoute const& route ) {
        return are_all_stops_identical( route );
      };

  w.add_land_route_1();
  w.add_land_route_2();
  w.add_sea_route_1();
  w.add_double_harbor_route();

  REQUIRE_FALSE( f( w.trade_routes().routes.at( 1 ) ) );
  REQUIRE( f( w.trade_routes().routes.at( 2 ) ) );
  REQUIRE_FALSE( f( w.trade_routes().routes.at( 3 ) ) );
  REQUIRE( f( w.trade_routes().routes.at( 4 ) ) );
}

TEST_CASE(
    "[trade-route] convert_trade_route_target_to_goto_target" ) {
  world w;
  w.create_colonies();
  auto const& token = w.token();
  TradeRouteTarget target;
  goto_target expected;

  auto const f = [&] [[clang::noinline]] {
    return convert_trade_route_target_to_goto_target(
        w.ss(), w.default_player(), target, token );
  };

  target   = TradeRouteTarget::harbor{};
  expected = goto_target::harbor{};
  REQUIRE( f() == expected );

  target   = TradeRouteTarget::colony{ .colony_id = 2 };
  expected = goto_target::map{ .tile = { .x = 6, .y = 1 } };
  REQUIRE( f() == expected );
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

  auto const f = [&] [[clang::noinline]] ( Unit const& unit ) {
    return unit_has_reached_trade_route_stop( w.ss(), unit );
  };

  w.add_land_route_1();
  w.add_sea_route_1();

  SECTION( "headed to colony, not arrived" ) {
    Unit& unit    = w.add_unit_on_map( e_unit_type::wagon_train,
                                       { .x = 1, .y = 2 } );
    unit.orders() = unit_orders::trade_route{
      .id = 1, .en_route_to_stop = 1 };
    REQUIRE_FALSE( f( unit ) );
  }

  SECTION( "headed to colony, arrived" ) {
    Unit& unit    = w.add_unit_on_map( e_unit_type::wagon_train,
                                       { .x = 6, .y = 1 } );
    unit.orders() = unit_orders::trade_route{
      .id = 1, .en_route_to_stop = 1 };
    REQUIRE( f( unit ) );
  }

  SECTION( "headed to colony, colony non-existent" ) {
    Unit& unit    = w.add_unit_on_map( e_unit_type::wagon_train,
                                       { .x = 6, .y = 1 } );
    unit.orders() = unit_orders::trade_route{
      .id = 1, .en_route_to_stop = 1 };
    destroy_colony( w.ss(), w.ts(), *get<1>( w.colonies_ ) );
    REQUIRE_FALSE( f( unit ) );
  }

  SECTION( "headed to colony from harbor" ) {
    Unit& unit    = w.add_unit_in_port( e_unit_type::caravel );
    unit.orders() = unit_orders::trade_route{
      .id = 2, .en_route_to_stop = 2 };
    REQUIRE_FALSE( f( unit ) );
  }

  SECTION( "headed to harbor, not arrived" ) {
    Unit& unit    = w.add_unit_on_map( e_unit_type::caravel,
                                       { .x = 0, .y = 0 } );
    unit.orders() = unit_orders::trade_route{
      .id = 2, .en_route_to_stop = 1 };
    REQUIRE_FALSE( f( unit ) );
  }

  SECTION( "headed to harbor, arrived" ) {
    Unit& unit    = w.add_unit_in_port( e_unit_type::caravel );
    unit.orders() = unit_orders::trade_route{
      .id = 2, .en_route_to_stop = 1 };
    REQUIRE( f( unit ) );
  }

  SECTION( "headed somewhere, route doesn't exist" ) {
    Unit& unit    = w.add_unit_in_port( e_unit_type::caravel );
    unit.orders() = unit_orders::trade_route{
      .id = 5, .en_route_to_stop = 1 };
    REQUIRE_FALSE( f( unit ) );
  }
}

TEST_CASE( "[trade-route] ask_edit_trade_route" ) {
  using enum e_player;
  world w;

  auto const f =
      [&] [[clang::noinline]] ( Player const& player ) {
        return co_await_test(
            ask_edit_trade_route( w.ss(), player, w.gui() ) );
      };

  w.gui().EXPECT__message_box(
      "You have not yet defined any trade routes." );
  REQUIRE( f( w.english() ) == nothing );

  w.add_land_route_1();
  w.add_land_route_2();
  w.add_sea_route_1();

  w.trade_routes().routes.at( 2 ).player = french;

  ChoiceConfig config;

  config = {
    .msg     = "Select Trade Route to Edit",
    .options = { { .key = "1", .display_name = "1. land.1" },
                 { .key = "3", .display_name = "2. sea.3" } } };
  w.gui().EXPECT__choice( config ).returns( nothing );
  REQUIRE( f( w.english() ) == nothing );

  w.gui().EXPECT__choice( config ).returns( "3" );
  REQUIRE( f( w.english() ) == 3 );

  config = {
    .msg     = "Select Trade Route to Edit",
    .options = { { .key = "2", .display_name = "1. land.2" } } };
  w.gui().EXPECT__choice( config ).returns( nothing );
  REQUIRE( f( w.french() ) == nothing );

  w.gui().EXPECT__choice( config ).returns( "2" );
  REQUIRE( f( w.french() ) == 2 );
}

TEST_CASE( "[trade-route] ask_delete_trade_route" ) {
  using enum e_player;
  world w;

  auto const f =
      [&] [[clang::noinline]] ( Player const& player ) {
        return co_await_test(
            ask_delete_trade_route( w.ss(), player, w.gui() ) );
      };

  w.gui().EXPECT__message_box(
      "You have not yet defined any trade routes." );
  REQUIRE( f( w.english() ) == nothing );

  w.add_land_route_1();
  w.add_land_route_2();
  w.add_sea_route_1();

  w.trade_routes().routes.at( 2 ).player = french;

  ChoiceConfig config;

  config = {
    .msg     = "Select Trade Route to Delete",
    .options = { { .key = "1", .display_name = "1. land.1" },
                 { .key = "3", .display_name = "2. sea.3" } } };
  w.gui().EXPECT__choice( config ).returns( nothing );
  REQUIRE( f( w.english() ) == nothing );

  w.gui().EXPECT__choice( config ).returns( "3" );
  REQUIRE( f( w.english() ) == 3 );

  config = {
    .msg     = "Select Trade Route to Delete",
    .options = { { .key = "2", .display_name = "1. land.2" } } };
  w.gui().EXPECT__choice( config ).returns( nothing );
  REQUIRE( f( w.french() ) == nothing );

  w.gui().EXPECT__choice( config ).returns( "2" );
  REQUIRE( f( w.french() ) == 2 );
}

TEST_CASE( "[trade-route] ask_create_trade_route" ) {
  world w;
}

TEST_CASE( "[trade-route] create_trade_route_object" ) {
  using enum e_player;
  using enum e_trade_route_type;
  world w;
  CreateTradeRoute params;
  TradeRoute expected;

  auto const f = [&] [[clang::noinline]] {
    return create_trade_route_object( w.trade_routes(), params );
  };

  params = {
    .name   = "some route",
    .type   = land,
    .player = english,
    .stop1  = TradeRouteTarget::harbor{},
    .stop2  = TradeRouteTarget::colony{ .colony_id = 3 },
  };
  expected = {
    .id     = 1,
    .name   = "some route",
    .player = english,
    .type   = land,
    .stops  = { { .target = TradeRouteTarget::harbor{} },
                { .target = TradeRouteTarget::colony{ .colony_id =
                                                         3 } } },
  };
  REQUIRE( f() == expected );

  params = {
    .name   = "some route 2",
    .type   = sea,
    .player = french,
    .stop1  = TradeRouteTarget::colony{ .colony_id = 4 },
    .stop2  = TradeRouteTarget::harbor{},
  };
  expected = {
    .id     = 2,
    .name   = "some route 2",
    .player = french,
    .type   = sea,
    .stops = { { .target = TradeRouteTarget::colony{ .colony_id =
                                                         4 } },
               { .target = TradeRouteTarget::harbor{} } },
  };
  REQUIRE( f() == expected );
}

TEST_CASE( "[trade-route] confirm_delete_trade_route" ) {
  world w;

  auto const f =
      [&] [[clang::noinline]] ( TradeRouteId const route_id ) {
        return co_await_test( confirm_delete_trade_route(
            w.ss().as_const, w.gui(), route_id ) );
      };

  w.add_land_route_1();
  w.add_land_route_2();
  w.add_sea_route_1();

  ChoiceConfig const config = {
    .msg =
        "Are you sure that you want to delete the [land.2] "
        "trade route?",
    .options = { { .key = "no", .display_name = "No" },
                 { .key = "yes", .display_name = "Yes" } } };

  w.gui().EXPECT__choice( config ).returns( nothing );
  REQUIRE_FALSE( f( 2 ) );

  w.gui().EXPECT__choice( config ).returns( "yes" );
  REQUIRE( f( 2 ) );
}

TEST_CASE( "[trade-route] delete_trade_route" ) {
  world w;

  w.add_land_route_1();
  w.add_land_route_2();
  w.add_sea_route_1();
  REQUIRE( w.trade_routes().routes.size() == 3 );

  REQUIRE( w.trade_routes().routes.contains( 1 ) );
  REQUIRE( w.trade_routes().routes.contains( 2 ) );
  REQUIRE( w.trade_routes().routes.contains( 3 ) );
  REQUIRE( w.trade_routes().routes.size() == 3 );

  delete_trade_route( w.trade_routes(), 2 );

  REQUIRE( w.trade_routes().routes.contains( 1 ) );
  REQUIRE_FALSE( w.trade_routes().routes.contains( 2 ) );
  REQUIRE( w.trade_routes().routes.contains( 3 ) );
  REQUIRE( w.trade_routes().routes.size() == 2 );

  delete_trade_route( w.trade_routes(), 2 );

  REQUIRE( w.trade_routes().routes.contains( 1 ) );
  REQUIRE_FALSE( w.trade_routes().routes.contains( 2 ) );
  REQUIRE( w.trade_routes().routes.contains( 3 ) );
  REQUIRE( w.trade_routes().routes.size() == 2 );

  delete_trade_route( w.trade_routes(), 1 );

  REQUIRE_FALSE( w.trade_routes().routes.contains( 1 ) );
  REQUIRE_FALSE( w.trade_routes().routes.contains( 2 ) );
  REQUIRE( w.trade_routes().routes.contains( 3 ) );
  REQUIRE( w.trade_routes().routes.size() == 1 );

  delete_trade_route( w.trade_routes(), 3 );

  REQUIRE_FALSE( w.trade_routes().routes.contains( 1 ) );
  REQUIRE_FALSE( w.trade_routes().routes.contains( 2 ) );
  REQUIRE_FALSE( w.trade_routes().routes.contains( 3 ) );
  REQUIRE( w.trade_routes().routes.size() == 0 );
}

TEST_CASE( "[trade-route] available_colonies_for_route" ) {
  world w;
}

TEST_CASE( "[trade-route] name_for_target" ) {
  using enum e_player;
  world w;
  TradeRouteTarget target;
  auto const& token = w.token();
  e_player player   = {};

  auto const f = [&] [[clang::noinline]] {
    return name_for_target( w.ss().as_const, w.player( player ),
                            target, token );
  };

  player = english;
  target = TradeRouteTarget::harbor{};
  REQUIRE( f() == "London" );

  player = french;
  target = TradeRouteTarget::harbor{};
  REQUIRE( f() == "La Rochelle" );

  w.create_colonies();

  player = english;
  target = TradeRouteTarget::colony{ .colony_id = 2 };
  REQUIRE( f() == "2" );

  get<1>( w.colonies_ )->name = "my colony";
  player                      = english;
  target = TradeRouteTarget::colony{ .colony_id = 2 };
  REQUIRE( f() == "my colony" );
}

TEST_CASE(
    "[trade-route] find_eligible_trade_routes_for_unit" ) {
  using enum e_player;
  using enum e_unit_type;
  world w;
  vector<TradeRouteId> expected;

  Unit const& ship =
      w.add_unit_on_map( caravel, { .x = 0, .y = 0 } );
  Unit const& foreign_ship =
      w.add_unit_on_map( caravel, { .x = 0, .y = 1 }, french );
  Unit const& wagon =
      w.add_unit_on_map( wagon_train, { .x = 2, .y = 0 } );
  Unit const& foreign_wagon = w.add_unit_on_map(
      wagon_train, { .x = 2, .y = 1 }, french );

  auto const f = [&] [[clang::noinline]] ( Unit const& unit ) {
    return find_eligible_trade_routes_for_unit( w.ss().as_const,
                                                unit );
  };

  // No trade routes.
  expected = {};
  REQUIRE( f( ship ) == expected );
  REQUIRE( f( foreign_ship ) == expected );
  REQUIRE( f( wagon ) == expected );
  REQUIRE( f( foreign_wagon ) == expected );

  w.add_land_route_1();
  w.add_sea_route_1();
  w.add_land_route_1();
  w.add_sea_route_1();

  expected = { 2, 4 };
  REQUIRE( f( ship ) == expected );
  expected = {};
  REQUIRE( f( foreign_ship ) == expected );
  expected = { 1, 3 };
  REQUIRE( f( wagon ) == expected );
  expected = {};
  REQUIRE( f( foreign_wagon ) == expected );

  w.trade_routes().routes.at( 2 ).player = french;
  w.trade_routes().routes.at( 3 ).player = french;

  expected = { 4 };
  REQUIRE( f( ship ) == expected );
  expected = { 2 };
  REQUIRE( f( foreign_ship ) == expected );
  expected = { 1 };
  REQUIRE( f( wagon ) == expected );
  expected = { 3 };
  REQUIRE( f( foreign_wagon ) == expected );
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
