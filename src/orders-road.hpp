/****************************************************************
**orders-road.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-25.
*
* Description: Carries out orders to build a road.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "orders.hpp"

namespace rn {

struct SS;
struct TS;

std::unique_ptr<OrdersHandler> handle_orders(
    SS& ss, TS& ts, UnitId id, orders::road const& road );

} // namespace rn
