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
#include "native-owned.hpp"
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
  RoadHandler( SS& ss, TS& ts, Player& player, UnitId unit_id )
    : ss_( ss ),
      ts_( ts ),
      player_( player ),
      unit_id_( unit_id ) {}

  wait<bool> confirm() override {
    Unit const& unit = ss_.units.unit_for( unit_id_ );
    if( unit.type() == e_unit_type::hardy_colonist ) {
      co_await ts_.gui.message_box(
          "This [Hardy Pioneer] requires at least 20 tools "
          "to build a road." );
      co_return false;
    }
    if( unit.type() != e_unit_type::pioneer &&
        unit.type() != e_unit_type::hardy_pioneer ) {
      co_await ts_.gui.message_box(
          "Only [Pioneers] and [Hardy Pioneers] can "
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
    Coord const tile = ss_.units.coord_for( unit_id_ );
    CHECK( ss_.terrain.is_land( tile ) );
    if( has_road( ss_.terrain, tile ) ) {
      co_await ts_.gui.message_box(
          "There is already a road on this square." );
      co_return false;
    }
    if( is_land_native_owned( ss_, player_, tile ) ) {
      bool const land_acquired =
          co_await prompt_player_for_taking_native_land(
              ss_, ts_, player_, tile,
              e_native_land_grab_type::build_road );
      if( !land_acquired ) {
        // In the OG the player loses its movement points if it
        // decided to retract the request after being presented
        // with the native-owned land options, but we don't do
        // that here since we don't expend movement points when a
        // pioneer begins to build a road, so that would be in-
        // consistent.
        co_return false;
      }
      // The player has acquired the land from the natives
      // through some means.
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

  SS&     ss_;
  TS&     ts_;
  Player& player_;
  UnitId  unit_id_;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<OrdersHandler> handle_orders( SS& ss, TS& ts,
                                         Player& player,
                                         UnitId  id,
                                         orders::road const& ) {
  return make_unique<RoadHandler>( ss, ts, player, id );
}

} // namespace rn
