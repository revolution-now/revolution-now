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
struct Unit;

enum class e_unit_type;

/****************************************************************
** Querying.
*****************************************************************/
maybe<TradeRoute&> look_up_trade_route(
    TradeRouteState& trade_routes, TradeRouteId id );

maybe<TradeRouteStop const&> look_up_trade_route_stop(
    TradeRoute const& route, int stop );

maybe<TradeRouteStop const&> look_up_next_trade_route_stop(
    TradeRoute const& route, int curr_stop );

maybe<TradeRouteTarget const&> curr_trade_route_target(
    TradeRouteState const& trade_routes, Unit const& unit );

[[nodiscard]] bool are_all_stops_identical(
    TradeRoute const& route );

/****************************************************************
** Unit Assignments.
*****************************************************************/
[[nodiscard]] bool unit_can_start_trade_route(
    e_unit_type type );

[[nodiscard]] bool unit_has_reached_trade_route_stop(
    SSConst const& ss, Unit const& unit );

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

[[nodiscard]] TradeRoute create_trade_route_object(
    TradeRouteState& trade_routes,
    CreateTradeRoute const& params );

wait<base::NoDiscard<bool>> confirm_delete_trade_route(
    SSConst const& ss, IGui& gui, TradeRouteId trade_route_id );

void delete_trade_route( TradeRouteState& trade_routes,
                         TradeRouteId trade_route_id );

/****************************************************************
** Trade Route Editing.
*****************************************************************/
// This will give the list of valid colonies that can be added to
// a trade route. The list will depend on the trade route type as
// well as any existing colony targets within it.
std::vector<ColonyId> available_colonies_for_route(
    SSConst const& ss, Player const& player,
    TerrainConnectivity const& connectivity,
    TradeRoute const& route );

std::string name_for_target( SSConst const& ss,
                             Player const& player,
                             TradeRouteTarget const& target );

/****************************************************************
** Trade Route Orders.
*****************************************************************/
std::vector<TradeRouteId> find_eligible_trade_routes_for_unit(
    SSConst const& ss, Unit const& unit );

wait<maybe<TradeRouteId>> select_trade_route(
    SSConst const& ss, Unit const& unit, IGui& gui,
    std::vector<TradeRouteId> const& route_ids );

wait<maybe<int>> ask_first_stop( SSConst const& ss,
                                 Player const& player, IGui& gui,
                                 TradeRouteId const route_id );

wait<maybe<TradeRouteOrdersConfirmed>>
confirm_trade_route_orders( SSConst const& ss,
                            Player const& player,
                            Unit const& unit, IGui& gui );

} // namespace rn
