/****************************************************************
**land-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-29.
*
* Description: Handles the main game view of the land.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "fmt-helper.hpp"
#include "id.hpp"
#include "orders.hpp"
#include "waitable.hpp"

// Rnl
#include "rnl/land-view.hpp"

namespace rn {

waitable<> landview_ensure_visible( Coord const& coord );
waitable<> landview_ensure_visible( UnitId id );

waitable<LandViewPlayerInput_t> landview_get_next_input(
    UnitId id );

waitable<> landview_end_of_turn();

waitable<> landview_animate_move( UnitId      id,
                                  e_direction direction );

enum class e_depixelate_anim { none, death, demote };

waitable<> landview_animate_attack( UnitId attacker,
                                    UnitId defender,
                                    bool   attacker_wins,
                                    e_depixelate_anim dp_anim );

struct Plane;
Plane* land_view_plane();

} // namespace rn
