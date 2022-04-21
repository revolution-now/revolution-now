/****************************************************************
**orders-road.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-25.
*
* Description: Carries out orders to build a road.
*
*****************************************************************/
#include "orders-road.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "game-state.hpp"
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "logger.hpp"
#include "road.hpp"
#include "window.hpp"

using namespace std;

namespace rn {

namespace {

struct RoadHandler : public OrdersHandler {
  RoadHandler( UnitId unit_id_ ) : unit_id( unit_id_ ) {}

  wait<bool> confirm() override {
    UnitsState const& units_state = GameState::units();
    Unit const&       unit = units_state.unit_for( unit_id );
    if( unit.type() == e_unit_type::hardy_colonist ) {
      co_await ui::message_box_basic(
          "This @[H]Hardy Pioneer@[] requires at least 20 tools "
          "to build a road." );
      co_return false;
    }
    if( unit.type() != e_unit_type::pioneer &&
        unit.type() != e_unit_type::hardy_pioneer ) {
      co_await ui::message_box_basic(
          "Only @[H]Pioneers@[] and @[H]Hardy Pioneers@[] can "
          "build roads." );
      co_return false;
    }
    UnitOwnership_t const& ownership =
        units_state.ownership_of( unit_id );
    if( !ownership.is<UnitOwnership::world>() ) {
      // This can happen if a pioneer is on a ship asking for or-
      // ders and it is given road-building orders.
      co_await ui::message_box_basic(
          "Roads can only be built while directly on a land "
          "tile." );
      co_return false;
    }
    Coord world_square = units_state.coord_for( unit_id );
    TerrainState const& terrain_state = GameState::terrain();
    CHECK( terrain_state.is_land( world_square ) );
    if( has_road( terrain_state, world_square ) ) {
      co_await ui::message_box_basic(
          "There is already a road on this square." );
      co_return false;
    }
    co_return true;
  }

  wait<> perform() override {
    lg.info( "building a road." );
    UnitsState& units_state = GameState::units();
    Unit&       unit        = units_state.unit_for( unit_id );
    // The unit of course does not need movement points to build
    // a road, but we use those to also track if the unit has
    // used up its turn.
    CHECK( !unit.mv_pts_exhausted() );
    // Note that we don't charge the unit any movement points
    // yet. That way the player can change their mind after
    // building a road and move the unit. They are only charged
    // movement points at the start of the next turn when they
    // contribute some progress to building the road.
    unit.build_road();
    unit.set_turns_worked( 0 );
    co_return;
  }

  UnitId unit_id;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<OrdersHandler> handle_orders(
    UnitId id, orders::road const& /*road*/ ) {
  return make_unique<RoadHandler>( id );
}

} // namespace rn
