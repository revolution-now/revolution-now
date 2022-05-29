/****************************************************************
**orders-move.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out orders wherein a unit is asked to move
*              onto an adjacent square.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "orders.hpp"

namespace rn {

struct IMapUpdater;
struct SettingsState;

std::unique_ptr<OrdersHandler> handle_orders(
    UnitId id, orders::move const& mv, IMapUpdater* map_updater,
    IGui& gui, SettingsState const& settings );

} // namespace rn
