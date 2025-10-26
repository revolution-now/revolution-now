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
  TradeRouteHandler( SS& ss, UnitId const unit_id )
    : unit_( ss.units.unit_for( unit_id ) ) {}

  wait<bool> confirm() override {
    TradeRouteId trade_route_id = {};
    int first_stop              = {};

    trade_route_orders_ = { .id               = trade_route_id,
                            .en_route_to_stop = first_stop };
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
  Unit& unit_;
  unit_orders::trade_route trade_route_orders_;
};

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> handle_command(
    IEngine&, SS& ss, TS&, IAgent&, Player&, UnitId id,
    command::trade_route const& ) {
  return make_unique<TradeRouteHandler>( ss, id );
}

} // namespace rn
