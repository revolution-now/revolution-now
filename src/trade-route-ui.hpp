/****************************************************************
**trade-route-ui.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Shows the Trade Route editing UI screen.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/trade-route-id.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IEngine;
struct IGui;
struct Planes;
struct SSConst;
struct Player;
struct TradeRouteState;

/****************************************************************
** Trade Route UI
*****************************************************************/
// Shows the full-screen Trade Route editing UI.
wait<> show_trade_route_edit_ui(
    IEngine& engine, SSConst const& ss, Player const& player,
    IGui& gui, Planes& planes,
    TradeRouteState& trade_route_state,
    TradeRouteId trade_route_id );

} // namespace rn
