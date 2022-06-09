/****************************************************************
**orders-plow.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-27.
*
* Description: Carries out orders to plow.
*
*****************************************************************/
#include "orders-plow.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "logger.hpp"
#include "map-square.hpp"
#include "plow.hpp"
#include "window.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

struct PlowHandler : public OrdersHandler {
  PlowHandler( IGui& gui_arg, UnitId unit_id_arg,
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
          "to plow." );
      co_return false;
    }
    if( unit.type() != e_unit_type::pioneer &&
        unit.type() != e_unit_type::hardy_pioneer ) {
      co_await gui.message_box(
          "Only @[H]Pioneers@[] and @[H]Hardy Pioneers@[] can "
          "plow." );
      co_return false;
    }
    UnitOwnership_t const& ownership =
        static_cast<UnitsState const&>( units_state )
            .ownership_of( unit_id );
    if( !ownership.is<UnitOwnership::world>() ) {
      // This can happen if a pioneer is on a ship asking for or-
      // ders and it is given plowing orders.
      co_await gui.message_box(
          "Plowing can only be done while directly on a land "
          "tile." );
      co_return false;
    }
    Coord world_square = units_state.coord_for( unit_id );
    CHECK( terrain_state.is_land( world_square ) );
    if( !can_plow( terrain_state, world_square ) ) {
      co_await gui.message_box(
          "@[H]{}@[] tiles cannot be plowed or cleared.",
          effective_terrain(
              terrain_state.square_at( world_square ) ) );
      co_return false;
    }
    if( has_irrigation( terrain_state, world_square ) ) {
      co_await gui.message_box(
          "There is already irrigation on this square." );
      co_return false;
    }
    co_return true;
  }

  wait<> perform() override {
    lg.info( "plowing." );
    Unit& unit = units_state.unit_for( unit_id );
    // The unit of course does not need movement points to plow
    // but we use those to also track if the unit has used up its
    // turn.
    CHECK( !unit.mv_pts_exhausted() );
    // Note that we don't charge the unit any movement points
    // yet. That way the player can change their mind after
    // plowing and move the unit. They are only charged movement
    // points at the start of the next turn when they contribute
    // some progress to plowing.
    unit.plow();
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
    UnitId id, orders::plow const& /*plow*/, IMapUpdater*,
    IGui& gui, Player&, TerrainState const& terrain_state,
    UnitsState& units_state, ColoniesState&,
    SettingsState const&, LandViewPlane&, Planes& ) {
  return make_unique<PlowHandler>( gui, id, terrain_state,
                                   units_state );
}

} // namespace rn
