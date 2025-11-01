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
struct TerrainConnectivity;
struct TradeRoute;
struct TradeRouteState;

/****************************************************************
** Trade Route UI
*****************************************************************/
// Shows the full-screen Trade Route editing UI.
wait<> show_trade_route_edit_ui(
    IEngine& engine, SSConst const& ss, Player const& player,
    IGui& gui, TerrainConnectivity const& connectivity,
    Planes& planes, TradeRouteState& trade_route_state,
    TradeRouteId trade_route_id );

// The new trade route should have a valid/unique ID, but should
// not yet have been added into the list of routes.
wait<> show_trade_route_create_ui(
    IEngine& engine, SSConst const& ss, Player const& player,
    IGui& gui, TerrainConnectivity const& connectivity,
    Planes& planes, TradeRouteState& trade_route_state,
    TradeRoute const& new_trade_route );

} // namespace rn
