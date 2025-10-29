/****************************************************************
**trade-route.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Handles things related to trade routes.
*
*****************************************************************/
#pragma once

// rds
#include "trade-route.rds.hpp"

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/trade-route-id.hpp"

// base
#include "base/vocab.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IGui;
struct Player;
struct SSConst;
struct TerrainConnectivity;
struct TradeRouteState;

enum class e_unit_type;

/****************************************************************
** Unit Assignments.
*****************************************************************/
[[nodiscard]] bool unit_can_start_trade_route(
    e_unit_type type );

/****************************************************************
** Create/Edit/Delete trade routes.
*****************************************************************/
wait<maybe<TradeRouteId>> ask_edit_trade_route(
    SSConst const& ss, Player const& player, IGui& gui );

wait<maybe<TradeRouteId>> ask_delete_trade_route(
    SSConst const& ss, Player const& player, IGui& gui );

wait<maybe<CreateTradeRoute>> ask_create_trade_route(
    SSConst const& ss, Player const& player, IGui& gui,
    TerrainConnectivity const& connectivity );

[[nodiscard]] TradeRouteId create_trade_route(
    TradeRouteState& trade_routes,
    CreateTradeRoute const& params );

wait<base::NoDiscard<bool>> confirm_delete_trade_route(
    SSConst const& ss, IGui& gui, TradeRouteId trade_route_id );

void delete_trade_route( TradeRouteState& trade_routes,
                         TradeRouteId trade_route_id );

} // namespace rn
