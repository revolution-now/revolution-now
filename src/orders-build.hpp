/****************************************************************
**orders-build.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out orders to build a colony
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "orders.hpp"

namespace rn {

struct IMapUpdater;

std::unique_ptr<OrdersHandler> handle_orders(
    UnitId id, orders::build const& build,
    IMapUpdater* map_updater );

} // namespace rn
