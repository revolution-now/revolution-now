/****************************************************************
**command-trade.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Carries out the command to start a trade route.
*
*****************************************************************/
#include "command-trade.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "igui.hpp"
#include "trade-route.hpp"
#include "ts.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** TradeRouteHandler
*****************************************************************/
struct TradeRouteHandler : public CommandHandler {
  TradeRouteHandler( SSConst const& ss, Player const& player,
                     Unit& unit, IGui& gui )
    : ss_( ss ), player_( player ), unit_( unit ), gui_( gui ) {}

  wait<bool> confirm() override {
    if( !unit_can_start_trade_route( unit_.type() ) ) {
      // Not expected to happen since the land-view should not
      // allow trade route orders to be assigned to this unit,
      // but just in case...
      co_await gui_.message_box(
          "[{}] cannot carry out trade routes.",
          unit_.desc().name_plural );
      co_return false;
    }

    if( ss_.trade_routes.routes.empty() ) {
      co_await gui_.message_box(
          "We have not yet defined any trade routes." );
      co_return false;
    }

    vector<TradeRouteId> const routes =
        find_eligible_trade_routes_for_unit( ss_, unit_ );
    if( routes.empty() ) {
      co_await gui_.message_box(
          "This [{}] is not eligible for any of the trade "
          "routes that we have defined.",
          unit_.desc().name );
      co_return false;
    }

    auto const route_id =
        co_await select_trade_route( ss_, unit_, gui_, routes );
    if( !route_id.has_value() )
      // Cancelled by the player.
      co_return false;

    TradeRouteId const trade_route_id = *route_id;

    auto const first_stop = co_await ask_first_stop(
        ss_, player_, gui_, trade_route_id );
    if( !first_stop.has_value() )
      // Either this route has no stops or the player cancelled.
      co_return false;

    trade_route_orders_ = { .id               = trade_route_id,
                            .en_route_to_stop = *first_stop };
    co_return true;
  }

  wait<> perform() override {
    // Note there is no charge of movement points for changing to
    // the trade route state; those are only subtracted as the
    // unit moves.
    unit_.orders() = trade_route_orders_;
    co_return;
  }

 private:
  SSConst const& ss_;
  Player const& player_;
  Unit& unit_;
  IGui& gui_;
  unit_orders::trade_route trade_route_orders_;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    IEngine&, SS& ss, TS& ts, IAgent&, Player& player,
    UnitId const unit_id, command::trade_route const& ) {
  return make_unique<TradeRouteHandler>(
      ss.as_const, as_const( player ),
      ss.units.unit_for( unit_id ), ts.gui );
}

} // namespace rn
