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
#include "combat.hpp"
#include "job.hpp"
#include "macros.hpp"
#include "travel.hpp"

// C++ standard library
#include <variant>

namespace rn {

using PlayerIntent = std::variant<
    // Orders about orders
    MetaAnalysis,
    // Jobs performed without moving, such as fortify, sentry,
    // plow field, build colony, etc.
    JobAnalysis,
    // If unit is to physical move to another square.
    TravelAnalysis,
    // If the move is toward a foreign unit
    CombatAnalysis>;
ASSERT_NOTHROW_MOVING( PlayerIntent );

Opt<PlayerIntent> player_intent( UnitId          id,
                                 orders_t const& orders );

bool confirm_explain( PlayerIntent const& analysis );

void affect_orders( PlayerIntent const& analysis );

std::vector<UnitId> units_to_prioritize(
    PlayerIntent const& analysis );

} // namespace rn
