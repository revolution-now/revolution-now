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
#include "ss/goto.rds.hpp"
#include "ss/trade-route-id.hpp"
#include "ss/unit-orders.rds.hpp"
#include "ss/unit.hpp"

// base
#include "base/vocab.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct Colony;
struct GotoRegistry;
struct IAgent;
struct IGui;
struct Player;
struct SS;
struct SSConst;
struct TerrainConnectivity;
struct TradeRouteState;
struct Unit;

enum class e_unit_type;

/****************************************************************
** Sanitization.
*****************************************************************/
// Unfortunately various things can change in the game that can
// invalidate the set of trade routes and/or trade route stops
// that are currently defined, and there are multiple reasons
// this can happen (e.g., colonies being abandoned, disbanded,
// captured, harbor being inaccessible, routes having no re-
// maining stops, cheat mode, manual save editing, etc.). This
// generally puts any code that reads and works with trade routes
// at risk for either check-failing or showing/doing inconsistent
// things. For this reason, any such code that requires a valid
// trade routes state requires a token that proves that the
// caller did the sanitization first before calling.

// This is a token that gets passed around to ensure that the
// trade route data has been sanitized before reading it. Saniti-
// zation removes stops associated with non-existent or inacces-
// sible targets, or empty routes.
struct [[nodiscard]] TradeRoutesSanitizedToken;

void validate_token( TradeRoutesSanitizedToken const& token );

// The actions vector will be cleared and populated with any ac-
// tions taken as a result of this function call.
[[nodiscard]] TradeRoutesSanitizedToken const&
sanitize_trade_routes(
    SSConst const& ss, Player const& player,
    TradeRouteState& trade_routes,
    std::vector<TradeRouteSanitizationAction>& actions_taken );

wait<> show_sanitization_actions(
    SSConst const&, IGui& gui, IAgent& agent,
    std::vector<TradeRouteSanitizationAction> const&
        actions_taken,
    TradeRoutesSanitizedToken const& token );

// This is the preferred method to use for sanitization when in
// an interactive context.
wait<std::reference_wrapper<TradeRoutesSanitizedToken const>>
run_trade_route_sanitization( SSConst const& ss,
                              Player const& player, IGui& gui,
                              TradeRouteState& trade_routes,
                              IAgent& agent );

// Returns nothing if the orders cannot be salvaged, e.g. if the
// route no longer exists.
maybe<unit_orders::trade_route> sanitize_unit_trade_route_orders(
    SSConst const& ss, Unit const& unit,
    TradeRoutesSanitizedToken const& token );

/****************************************************************
** Querying.
*****************************************************************/
maybe<TradeRoute&> look_up_trade_route(
    TradeRouteState& trade_routes, TradeRouteId id );

maybe<TradeRoute const&> look_up_trade_route(
    TradeRouteState const& trade_routes, TradeRouteId id );

maybe<TradeRouteStop const&> look_up_trade_route_stop(
    TradeRoute const& route, int stop );

maybe<TradeRouteStop const&> look_up_next_trade_route_stop(
    TradeRoute const& route, int curr_stop );

maybe<TradeRouteTarget const&> curr_trade_route_target(
    TradeRouteState const& trade_routes, Unit const& unit );

[[nodiscard]] bool are_all_stops_identical(
    TradeRoute const& route );

// Include player here because the sanitization process is player
// specific, so we want to make sure we're running in a
// player-specific context.
[[nodiscard]] goto_target
convert_trade_route_target_to_goto_target(
    SSConst const& ss, Player const& player,
    TradeRouteTarget const& trade_route_target,
    TradeRoutesSanitizedToken const& token );

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
[[nodiscard]] std::vector<ColonyId> available_colonies_for_route(
    SSConst const& ss, Player const& player,
    TerrainConnectivity const& connectivity,
    TradeRoute const& route );

// Check-fail if the target refers to a colony that no longer ex-
// ists.
[[nodiscard]] std::string name_for_target(
    SSConst const& ss, Player const& player,
    TradeRouteTarget const& target,
    TradeRoutesSanitizedToken const& token );

/****************************************************************
** Trade Route Orders.
*****************************************************************/
[[nodiscard]] std::vector<TradeRouteId>
find_eligible_trade_routes_for_unit( SSConst const& ss,
                                     Unit const& unit );

wait<maybe<TradeRouteId>> select_trade_route(
    SSConst const& ss, Unit const& unit, IGui& gui,
    std::vector<TradeRouteId> const& route_ids );

wait<maybe<int>> ask_first_stop(
    SSConst const& ss, Player const& player, IGui& gui,
    TradeRouteId const route_id,
    TradeRoutesSanitizedToken const& token );

wait<maybe<TradeRouteOrdersConfirmed>>
confirm_trade_route_orders(
    SSConst const& ss, Player const& player, Unit const& unit,
    IGui& gui, TradeRoutesSanitizedToken const& token );

/****************************************************************
** Load/Unload.
*****************************************************************/
void trade_route_unload( SS& ss, Player& player, Unit& unit,
                         TradeRouteStop const& stop );

void trade_route_load( SS& ss, Player& player, Unit& unit,
                       TradeRouteStop const& stop );

/****************************************************************
** End to end.
*****************************************************************/
EvolveTradeRoute evolve_trade_route_human(
    SS& ss, Player& player, GotoRegistry& goto_registry,
    TerrainConnectivity const& connectivity, Unit& unit );

} // namespace rn
