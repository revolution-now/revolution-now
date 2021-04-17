/****************************************************************
**orders-disband.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out orders to disband a unit.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "orders.hpp"

namespace rn {

std::unique_ptr<OrdersHandler> handle_orders(
    UnitId id, orders::disband const& disband );

} // namespace rn
