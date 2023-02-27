/****************************************************************
**command-plow.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-27.
*
* Description: Carries out commands to plow.
*
*****************************************************************/
#include "command-plow.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "logger.hpp"
#include "map-square.hpp"
#include "native-owned.hpp"
#include "plow.hpp"
#include "ts.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

struct PlowHandler : public CommandHandler {
  PlowHandler( SS& ss, TS& ts, Player& player, UnitId unit_id )
    : ss_( ss ),
      ts_( ts ),
      player_( player ),
      unit_id_( unit_id ) {}

  wait<bool> confirm() override {
    Unit const& unit = ss_.units.unit_for( unit_id_ );
    if( unit.type() == e_unit_type::hardy_colonist ) {
      co_await ts_.gui.message_box(
          "This [Hardy Pioneer] requires at least 20 tools "
          "to plow." );
      co_return false;
    }
    if( unit.type() != e_unit_type::pioneer &&
        unit.type() != e_unit_type::hardy_pioneer ) {
      co_await ts_.gui.message_box(
          "Only [Pioneers] and [Hardy Pioneers] can "
          "plow." );
      co_return false;
    }
    UnitOwnership const& ownership =
        static_cast<UnitsState const&>( ss_.units )
            .ownership_of( unit_id_ );
    if( !ownership.is<UnitOwnership::world>() ) {
      // This can happen if a pioneer is on a ship asking for or-
      // ders and it is given plowing commands.
      co_await ts_.gui.message_box(
          "Plowing can only be done while directly on a land "
          "tile." );
      co_return false;
    }
    Coord const tile = ss_.units.coord_for( unit_id_ );
    CHECK( ss_.terrain.is_land( tile ) );
    if( has_irrigation( ss_.terrain, tile ) ) {
      co_await ts_.gui.message_box(
          "There is already irrigation on this square." );
      co_return false;
    }
    if( !can_plow( ss_.terrain, tile ) ) {
      co_await ts_.gui.message_box(
          "[{}] tiles cannot be plowed or cleared.",
          effective_terrain( ss_.terrain.square_at( tile ) ) );
      co_return false;
    }
    // Don't allow a pioneer to start plowing or clearing on a
    // tile where there is already a pioneer working, otherwise
    // this permits the "pre-charged pioneer" exploit whereby two
    // pioneers can be told to clear a forest in parallel, then
    // they finish at the same time, and the first one to be
    // processed clears the forest and the second one plows the
    // terrain. This would effectively allows transitioning a
    // forest to irrigation in twice the time.
    if( has_pioneer_working( ss_, tile ) ) {
      co_await ts_.gui.message_box(
          "There is already a pioneer working on this tile." );
      co_return false;
    }
    if( is_land_native_owned( ss_, player_, tile ) ) {
      MapSquare const& square = ss_.terrain.square_at( tile );
      e_native_land_grab_type const type =
          ( square.overlay == e_land_overlay::forest )
              ? e_native_land_grab_type::clear_forest
              : e_native_land_grab_type::irrigate;
      bool const land_acquired =
          co_await prompt_player_for_taking_native_land(
              ss_, ts_, player_, tile, type );
      if( !land_acquired ) {
        // In the OG the player loses its movement points if it
        // decided to retract the request after being presented
        // with the native-owned land options, but we don't do
        // that here since we don't expend movement points when a
        // pioneer begins to plow successfully, so that would be
        // inconsistent.
        co_return false;
      }
      // The player has acquired the land from the natives
      // through some means.
    }
    co_return true;
  }

  wait<> perform() override {
    lg.info( "plowing." );
    Unit& unit = ss_.units.unit_for( unit_id_ );
    // The unit of course does not need movement points to plow
    // but we use those to also track if the unit has used up its
    // turn.
    CHECK( !unit.mv_pts_exhausted() );
    // Note that we don't charge the unit any movement points
    // yet. That way the player can change their mind after
    // plowing and move the unit. They are only charged movement
    // points at the start of the next turn when they contribute
    // some progress to plowing.
    unit.orders() = unit_orders::plow{ .turns_worked = 0 };
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
unique_ptr<CommandHandler> handle_command(
    SS& ss, TS& ts, Player& player, UnitId id,
    command::plow const& ) {
  return make_unique<PlowHandler>( ss, ts, player, id );
}

} // namespace rn
