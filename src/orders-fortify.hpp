/****************************************************************
**orders-fortify.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out orders to fortify or sentry a unit.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "orders.hpp"

namespace rn {

struct Player;
struct SS;
struct TS;

std::unique_ptr<OrdersHandler> handle_orders(
    SS& ss, TS& ts, Player& player, UnitId id,
    orders::fortify const& fortify );

std::unique_ptr<OrdersHandler> handle_orders(
    SS& ss, TS& ts, Player& player, UnitId id,
    orders::sentry const& sentry );

} // namespace rn
