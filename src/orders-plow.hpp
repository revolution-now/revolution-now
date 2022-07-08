/****************************************************************
**orders-plow.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-27.
*
* Description: Carries out orders to plow.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "orders.hpp"

namespace rn {

struct Planes;
struct SS;
struct TS;

std::unique_ptr<OrdersHandler> handle_orders(
    Planes& planes, SS& ss, TS& ts, UnitId id,
    orders::plow const& plow );

} // namespace rn
