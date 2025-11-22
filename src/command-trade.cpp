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
#include "agents.hpp"
#include "co-wait.hpp"
#include "trade-route.hpp"
#include "ts.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** TradeRouteHandler
*****************************************************************/
struct TradeRouteHandler : public CommandHandler {
  TradeRouteHandler( SS& ss, Player const& player, Unit& unit,
                     IAgent& agent, IGui& gui )
    : ss_( ss ),
      player_( player ),
      unit_( unit ),
      agent_( agent ),
      gui_( gui ) {}

  wait<bool> confirm() override {
    // Sanitize.
    TradeRoutesSanitizedToken const& token =
        co_await run_trade_route_sanitization(
            ss_.as_const, player_, gui_, ss_.trade_routes,
            agent_ );

    // Confirm.
    auto const route_orders =
        co_await confirm_trade_route_orders(
            ss_.as_const, player_, as_const( unit_ ), gui_,
            token );
    if( !route_orders.has_value() ) co_return false;

    trade_route_orders_ = {
      .id               = route_orders->id,
      .en_route_to_stop = route_orders->en_route_to_stop };
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
  SS& ss_;
  Player const& player_;
  Unit& unit_;
  IAgent& agent_;
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
      ss, as_const( player ), ss.units.unit_for( unit_id ),
      ts.agents()[player.type], ts.gui );
}

} // namespace rn
