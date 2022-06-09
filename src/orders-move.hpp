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

struct ColoniesState;
struct IMapUpdater;
struct LandViewPlane;
struct Player;
struct SettingsState;
struct TerrainState;
struct UnitsState;

std::unique_ptr<OrdersHandler> handle_orders(
    UnitId id, orders::move const& mv, IMapUpdater* map_updater,
    IGui& gui, Player& player, TerrainState const& terrain_state,
    UnitsState& units_state, ColoniesState& colonies_state,
    SettingsState const& settings,
    LandViewPlane&       land_view_plane );

} // namespace rn
