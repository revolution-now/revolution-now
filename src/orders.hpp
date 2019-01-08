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
#include "fmt-helper.hpp"
#include "geo-types.hpp"
#include "id.hpp"

// C++ standard library
#include <variant>

/****************************************************************
**Orders
*****************************************************************/
// An `Orders` is a general term describing what the player
// proposes that a unit do when the unit asks the player. Roughly
// speaking, it captures the input that the player gives when the
// unit is waiting for orders. This could include "wait", "goto
// tile X", "move to tile X", "forfeight movement points", "plow
// square", "build colony". Orders only represent what the player
// has proposed, and so it makes sense to talk about orders that
// are not permitted. For example, the player might give orders
// to a unit to "move left", but doing so might cause a land unit
// to go into a sea square without a ship, which would not be
// allowed. In that case, the orders are not allowed.

#define ORDER_MONOSTATE( __o )    \
  namespace rn::orders {          \
  struct __o##_t {};              \
  inline constexpr __o##_t __o{}; \
  }                               \
  DEFINE_FORMAT_( rn::orders::__o##_t, "{}", TO_STRING( __o ) );

// Define monostate orders with a single macro statement.

ORDER_MONOSTATE( quit ); // temporary until we can quit properly
ORDER_MONOSTATE( wait );
ORDER_MONOSTATE( forfeight );

// Define multi-state orders with the struct and formatting spec.

namespace rn::orders {

struct move {
  e_direction d;
};

} // namespace rn::orders

DEFINE_FORMAT( rn::orders::move, "move{{{{{}}}}}", o.d );

namespace rn {

using Orders = std::variant<orders::quit_t,
                            // defer to later in same turn
                            orders::wait_t,
                            // forfeight remainder of turn
                            orders::forfeight_t,
                            // moving on the map
                            orders::move>;

void        push_unit_orders( UnitId id, Orders const& orders );
Opt<Orders> pop_unit_orders( UnitId id );

} // namespace rn
