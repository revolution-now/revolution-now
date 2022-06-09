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
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "logger.hpp"
#include "road.hpp"
#include "window.hpp"

using namespace std;

namespace rn {

namespace {

struct RoadHandler : public OrdersHandler {
  RoadHandler( IGui& gui_arg, UnitId unit_id_arg,
               TerrainState const& terrain_state_arg,
               UnitsState&         units_state_arg )
    : gui( gui_arg ),
      unit_id( unit_id_arg ),
      terrain_state( terrain_state_arg ),
      units_state( units_state_arg ) {}

  wait<bool> confirm() override {
    Unit const& unit = units_state.unit_for( unit_id );
    if( unit.type() == e_unit_type::hardy_colonist ) {
      co_await gui.message_box(
          "This @[H]Hardy Pioneer@[] requires at least 20 tools "
          "to build a road." );
      co_return false;
    }
    if( unit.type() != e_unit_type::pioneer &&
        unit.type() != e_unit_type::hardy_pioneer ) {
      co_await gui.message_box(
          "Only @[H]Pioneers@[] and @[H]Hardy Pioneers@[] can "
          "build roads." );
      co_return false;
    }
    UnitOwnership_t const& ownership =
        static_cast<UnitsState const&>( units_state )
            .ownership_of( unit_id );
    if( !ownership.is<UnitOwnership::world>() ) {
      // This can happen if a pioneer is on a ship asking for or-
      // ders and it is given road-building orders.
      co_await gui.message_box(
          "Roads can only be built while directly on a land "
          "tile." );
      co_return false;
    }
    Coord world_square = units_state.coord_for( unit_id );
    CHECK( terrain_state.is_land( world_square ) );
    if( has_road( terrain_state, world_square ) ) {
      co_await gui.message_box(
          "There is already a road on this square." );
      co_return false;
    }
    co_return true;
  }

  wait<> perform() override {
    lg.info( "building a road." );
    Unit& unit = units_state.unit_for( unit_id );
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

  IGui&               gui;
  UnitId              unit_id;
  TerrainState const& terrain_state;
  UnitsState&         units_state;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<OrdersHandler> handle_orders(
    UnitId id, orders::road const& /*road*/, IMapUpdater*,
    IGui& gui, Player&, TerrainState const& terrain_state,
    UnitsState& units_state, ColoniesState&,
    SettingsState const&, LandViewPlane&, Planes& ) {
  return make_unique<RoadHandler>( gui, id, terrain_state,
                                   units_state );
}

} // namespace rn
