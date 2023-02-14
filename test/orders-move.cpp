/****************************************************************
**orders-move.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-25.
*
* Description: Unit tests for the src/orders-move.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/orders-move.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/icolony-viewer.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/land-view-plane.hpp"

// Revolution Now
#include "src/map-square.hpp"
#include "src/plane-stack.hpp"

// config
#include "src/config/unit-type.rds.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/player.hpp"
#include "ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;
using ::mock::matchers::StrContains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
    add_player( e_nation::dutch );
    add_player( e_nation::french );

    // This is so that we don't try to pop up a box telling the
    // player that they've discovered the new world.
    player( e_nation::dutch ).new_world_name  = "";
    player( e_nation::french ).new_world_name = "";
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
// This tests the case where a ship is on a land square adjacent
// to ocean and that it can move from the land square onto the
// ocean. This can happen if there is a ship in the port of a
// colony and the colony either gets abandoned or disappears due
// to starvation. In that case we need to make sure that the ship
// can move and that the orders handler e.g. won't check fail due
// to a ship on a land square not containing a colony.
TEST_CASE( "[orders-move] ship can move from land to ocean" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  World   W;
  Player& player = W.default_player();
  UnitId  id     = W.add_unit_on_map( e_unit_type::galleon,
                                      Coord{ .x = 1, .y = 1 } )
                  .id();
  // Sanity check to make sure we are testing what we think we're
  // testing.
  REQUIRE( is_land( W.square( W.units().coord_for( id ) ) ) );
  REQUIRE( W.units().coord_for( id ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( W.units().unit_for( id ).desc().ship );

  {
    // First make sure that it can't move from land to land.
    unique_ptr<OrdersHandler> handler =
        handle_orders( W.planes(), W.ss(), W.ts(), player, id,
                       orders::move{ .d = e_direction::n } );
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    REQUIRE( *w_confirm == false );
    REQUIRE( W.units().coord_for( id ) ==
             Coord{ .x = 1, .y = 1 } );
  }

  {
    // Now make sure that it can move from land to water.
    unique_ptr<OrdersHandler> handler =
        handle_orders( W.planes(), W.ss(), W.ts(), player, id,
                       orders::move{ .d = e_direction::ne } );
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    REQUIRE( *w_confirm == true );
    REQUIRE( W.units().coord_for( id ) ==
             Coord{ .x = 1, .y = 1 } );

    wait<> w_perform = handler->perform();
    REQUIRE( !w_perform.exception() );
    REQUIRE( w_perform.ready() );
    REQUIRE( W.units().coord_for( id ) ==
             Coord{ .x = 2, .y = 0 } );
  }
}

TEST_CASE(
    "[orders-move] consumption of movement points when moving "
    "into a colony" ) {
  World             W;
  MockLandViewPlane land_view_plane;
  W.planes().back().land_view = &land_view_plane;
  Player&       player        = W.default_player();
  Colony const& colony     = W.add_colony( { .x = 1, .y = 1 } );
  Unit const&   missionary = W.add_unit_on_map(
      e_unit_type::missionary, { .x = 0, .y = 1 } );
  Unit const& free_colonist = W.add_unit_on_map(
      e_unit_type::free_colonist, { .x = 2, .y = 1 } );
  Unit const& wagon_train = W.add_unit_on_map(
      e_unit_type::wagon_train, { .x = 1, .y = 0 } );
  Unit const& privateer = W.add_unit_on_map(
      e_unit_type::privateer, { .x = 0, .y = 0 } );

  auto move_unit = [&]( UnitId unit_id, e_direction d ) {
    land_view_plane.EXPECT__animate( _ ).returns<monostate>();
    unique_ptr<OrdersHandler> handler =
        handle_orders( W.planes(), W.ss(), W.ts(), player,
                       unit_id, orders::move{ .d = d } );
    wait<OrdersHandlerRunResult> const w = handler->run();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    OrdersHandlerRunResult const expected_result{
        .order_was_run = true, .units_to_prioritize = {} };
    REQUIRE( *w == expected_result );
  };

  // Allow the free colonist to move along a road. This tests
  // that a unit is allowed to expend less than one movement
  // point to enter a colony in the relevant situation.
  W.square( { .x = 1, .y = 1 } ).road = true;
  W.square( { .x = 2, .y = 1 } ).road = true;
  // This will make sure that the movement points required to
  // enter a colony square are capped at 1.
  W.square( { .x = 1, .y = 1 } ).overlay = e_land_overlay::hills;

  // Sanity check.
  REQUIRE( missionary.movement_points() == 2 );
  REQUIRE( free_colonist.movement_points() == 1 );
  REQUIRE( wagon_train.movement_points() == 2 );
  REQUIRE( privateer.movement_points() == 8 );
  REQUIRE( W.units().coord_for( missionary.id() ) ==
           Coord{ .x = 0, .y = 1 } );
  REQUIRE( W.units().coord_for( free_colonist.id() ) ==
           Coord{ .x = 2, .y = 1 } );
  REQUIRE( W.units().coord_for( wagon_train.id() ) ==
           Coord{ .x = 1, .y = 0 } );
  REQUIRE( W.units().coord_for( privateer.id() ) ==
           Coord{ .x = 0, .y = 0 } );

  move_unit( missionary.id(), e_direction::e );
  move_unit( free_colonist.id(), e_direction::w );
  move_unit( wagon_train.id(), e_direction::s );

  W.colony_viewer()
      .EXPECT__show( _, colony.id )
      .returns( e_colony_abandoned::no );
  move_unit( privateer.id(), e_direction::se );

  // No road; consumes one movement point.
  REQUIRE( missionary.movement_points() == 1 );

  // Road; consumes 1/3 point.
  REQUIRE( free_colonist.movement_points() ==
           MovementPoints::_2_3() );

  // Normal movement point consumption for wagon train, unlike OG
  // it doesn't forfeight all points when moving into a colony
  // square.
  REQUIRE( wagon_train.movement_points() == 1 );
  REQUIRE( privateer.movement_points() == 0 );
  REQUIRE( W.units().coord_for( missionary.id() ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( W.units().coord_for( free_colonist.id() ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( W.units().coord_for( wagon_train.id() ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( W.units().coord_for( privateer.id() ) ==
           Coord{ .x = 1, .y = 1 } );
}

TEST_CASE(
    "[orders-move] ship unloads units when moving into "
    "colony" ) {
  World             W;
  MockLandViewPlane land_view_plane;
  W.planes().back().land_view = &land_view_plane;
  Player&       player        = W.default_player();
  Colony const& colony = W.add_colony( { .x = 1, .y = 1 } );
  Unit const& galleon  = W.add_unit_on_map( e_unit_type::galleon,
                                            { .x = 0, .y = 0 } );
  Unit const& missionary = W.add_unit_in_cargo(
      e_unit_type::missionary, galleon.id() );
  Unit const& free_colonist = W.add_unit_in_cargo(
      e_unit_type::free_colonist, galleon.id() );

  auto move_unit = [&]( UnitId unit_id, e_direction d ) {
    land_view_plane.EXPECT__animate( _ ).returns<monostate>();
    unique_ptr<OrdersHandler> handler =
        handle_orders( W.planes(), W.ss(), W.ts(), player,
                       unit_id, orders::move{ .d = d } );
    wait<OrdersHandlerRunResult> const w = handler->run();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    return *w;
  };

  // Sanity check.
  W.colony_viewer()
      .EXPECT__show( _, colony.id )
      .returns( e_colony_abandoned::no );
  OrdersHandlerRunResult const expected_res{
      .order_was_run       = true,
      .units_to_prioritize = { missionary.id(),
                               free_colonist.id() } };
  OrdersHandlerRunResult const res =
      move_unit( galleon.id(), e_direction::se );
  REQUIRE( res == expected_res );

  REQUIRE( missionary.movement_points() == 2 );
  REQUIRE( free_colonist.movement_points() == 1 );
  REQUIRE( galleon.movement_points() == 0 );

  REQUIRE( W.units().coord_for( missionary.id() ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( W.units().coord_for( free_colonist.id() ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( W.units().coord_for( galleon.id() ) ==
           Coord{ .x = 1, .y = 1 } );
}

TEST_CASE(
    "[orders-move] unit on ship attempting to attack brave" ) {
  World W;
  Unit& caravel  = W.add_unit_on_map( e_unit_type::caravel,
                                      { .x = 0, .y = 0 } );
  Unit& colonist = W.add_unit_in_cargo(
      e_unit_type::free_colonist, caravel.id() );
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 0 }, e_tribe::cherokee );
  W.add_native_unit_on_map( e_native_unit_type::brave,
                            { .x = 1, .y = 1 }, dwelling.id );

  unique_ptr<OrdersHandler> handler = handle_orders(
      W.planes(), W.ss(), W.ts(), W.french(), colonist.id(),
      orders::move{ .d = e_direction::se } );
  W.gui()
      .EXPECT__message_box(
          "We cannot attack a land unit from a ship." )
      .returns<monostate>();
  wait<bool> w_confirm = handler->confirm();
  REQUIRE( !w_confirm.exception() );
  REQUIRE( w_confirm.ready() );
  REQUIRE_FALSE( *w_confirm );
}

} // namespace
} // namespace rn
