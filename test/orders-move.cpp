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
#include "test/mocks/igui.hpp"

// Revolution Now
#include "src/map-square.hpp"

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
  // This is so that we don't try to pop up a box telling the
  // player that they've discovered the new world.
  player.new_world_name = "";
  UnitId id = W.add_unit_on_map( e_unit_type::galleon,
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
