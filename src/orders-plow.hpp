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

struct IMapUpdater;

std::unique_ptr<OrdersHandler> handle_orders(
    UnitId id, orders::plow const& plow, IMapUpdater* );

} // namespace rn
