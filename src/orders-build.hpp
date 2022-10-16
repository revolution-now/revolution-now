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

struct Planes;
struct Player;
struct SS;
struct TS;

std::unique_ptr<OrdersHandler> handle_orders(
    Planes& planes, SS& ss, TS& ts, Player& player, UnitId id,
    orders::build const& build );

} // namespace rn
