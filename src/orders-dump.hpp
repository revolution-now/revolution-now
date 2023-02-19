/****************************************************************
**orders-dump.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-09.
*
* Description: Carries out orders to dump cargo overboard from
*              a ship or wagon train.
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
    orders::dump const& dump );

} // namespace rn
