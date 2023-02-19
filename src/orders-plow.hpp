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

struct Player;
struct SS;
struct TS;

std::unique_ptr<OrdersHandler> handle_orders(
    SS& ss, TS& ts, Player& player, UnitId id,
    orders::plow const& plow );

} // namespace rn
