/****************************************************************
**orders.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-27.
*
* Description: Handles the representation and application of
*              the orders that players can give to units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "adt.hpp"
#include "fmt-helper.hpp"
#include "geo-types.hpp"
#include "id.hpp"

// C++ standard library
#include <variant>

/****************************************************************
**Orders
*****************************************************************/
// An `orders_t` is a general term describing what the player
// proposes that a unit do when the unit asks the player. Roughly
// speaking, it captures the input that the player gives when the
// unit is waiting for orders. This could include "wait", "goto
// tile X", "move to tile X", "forfeight movement points", "plow
// square", "build colony". orders_t only represent what the
// player has proposed, and so it makes sense to talk about
// orders that are not permitted. For example, the player might
// give orders to a unit to "move left", but doing so might cause
// a land unit to go into a sea square without a ship, which
// would not be allowed. In that case, the orders are not
// allowed.
ADT( rn, orders,            //
     ( quit ),              //
     ( wait ),              //
     ( forfeight ),         //
     ( fortify ),           //
     ( sentry ),            //
     ( disband ),           //
     ( direction,           //
       ( e_direction, d ) ) //
);

namespace rn {

void push_unit_orders( UnitId id, orders_t const& orders );
Opt<orders_t> pop_unit_orders( UnitId id );

} // namespace rn
