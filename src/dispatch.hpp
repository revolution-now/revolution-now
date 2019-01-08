/****************************************************************
**dispatch.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-08.
*
* Description: Dispatches player orders to the right handler.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "analysis.hpp"
#include "travel.hpp"

// C++ standard library
#include <variant>

namespace rn {

using OrdersAnalysisDispatch = std::variant<
    // Orders about orders
    MetaAnalysis,
    // If unit is to physical move to another square.
    TravelAnalysis
    // If the move is toward a foreign unit
    // ProposedCombatAnalysisResult,
    //// actions in current tile
    // ProposedJobAnalysisResult>;
    >;

Opt<OrdersAnalysisDispatch> dispatch_orders(
    UnitId id, Orders const& orders );

bool confirm_explain( OrdersAnalysisDispatch const& analysis );
void affect_orders( OrdersAnalysisDispatch const& analysis );
std::vector<UnitId> units_to_prioritize(
    OrdersAnalysisDispatch const& analysis );

} // namespace rn
