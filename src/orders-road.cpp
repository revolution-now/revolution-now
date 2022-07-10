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
#include "logger.hpp"
#include "road.hpp"
#include "ts.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

struct RoadHandler : public OrdersHandler {
  RoadHandler( SS& ss, TS& ts, UnitId unit_id )
    : ss_( ss ), ts_( ts ), unit_id_( unit_id ) {}

  wait<bool> confirm() override {
    Unit const& unit = ss_.units.unit_for( unit_id_ );
    if( unit.type() == e_unit_type::hardy_colonist ) {
      co_await ts_.gui.message_box(
          "This @[H]Hardy Pioneer@[] requires at least 20 tools "
          "to build a road." );
      co_return false;
    }
    if( unit.type() != e_unit_type::pioneer &&
        unit.type() != e_unit_type::hardy_pioneer ) {
      co_await ts_.gui.message_box(
          "Only @[H]Pioneers@[] and @[H]Hardy Pioneers@[] can "
          "build roads." );
      co_return false;
    }
    UnitOwnership_t const& ownership =
        static_cast<UnitsState const&>( ss_.units )
            .ownership_of( unit_id_ );
    if( !ownership.is<UnitOwnership::world>() ) {
      // This can happen if a pioneer is on a ship asking for or-
      // ders and it is given road-building orders.
      co_await ts_.gui.message_box(
          "Roads can only be built while directly on a land "
          "tile." );
      co_return false;
    }
    Coord world_square = ss_.units.coord_for( unit_id_ );
    CHECK( ss_.terrain.is_land( world_square ) );
    if( has_road( ss_.terrain, world_square ) ) {
      co_await ts_.gui.message_box(
          "There is already a road on this square." );
      co_return false;
    }
    co_return true;
  }

  wait<> perform() override {
    lg.info( "building a road." );
    Unit& unit = ss_.units.unit_for( unit_id_ );
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

  SS&    ss_;
  TS&    ts_;
  UnitId unit_id_;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<OrdersHandler> handle_orders( Planes&, SS& ss, TS& ts,
                                         UnitId id,
                                         orders::road const& ) {
  return make_unique<RoadHandler>( ss, ts, id );
}

} // namespace rn
