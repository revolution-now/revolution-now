/****************************************************************
**map-view.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-27.
*
* Description: Logic for handling player interactions in the map
*              view, separate from the plane, for testability.
*
*****************************************************************/
#pragma once

// ss
#include "ss/colony-id.hpp"
#include "ss/unit-id.hpp"

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/maybe.hpp"

namespace rn {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct IVisibility;
struct SS;
struct SSConst;
struct TS;

/****************************************************************
** Map revealed.
*****************************************************************/
void reveal_entire_map( SS& ss, TS& ts );

/****************************************************************
** Colonies.
*****************************************************************/
base::maybe<ColonyId> can_open_colony_on_tile(
    IVisibility const& viz, gfx::point tile );

/****************************************************************
** Units.
*****************************************************************/
std::vector<UnitId> can_activate_units_on_tile(
    SSConst const& ss, IVisibility const& viz, gfx::point tile );

} // namespace rn
