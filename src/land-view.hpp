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
#include "coord.hpp"
#include "fmt-helper.hpp"
#include "id.hpp"
#include "waitable.hpp"

// Rnl
#include "rnl/land-view.hpp"
#include "rnl/orders.hpp"

namespace rn {

waitable<> landview_ensure_visible( Coord const& coord );
waitable<> landview_ensure_visible( UnitId id );

waitable<LandViewPlayerInput_t> landview_get_next_input(
    UnitId id );

waitable<LandViewPlayerInput_t> landview_eot_get_next_input();

waitable<> landview_animate_move( UnitId      id,
                                  e_direction direction );

enum class e_depixelate_anim { none, death, demote };

waitable<> landview_animate_attack( UnitId attacker,
                                    UnitId defender,
                                    bool   attacker_wins,
                                    e_depixelate_anim dp_anim );

waitable<> landview_animate_colony_capture( UnitId   attacker_id,
                                            UnitId   defender_id,
                                            ColonyId colony_id );

// Clear any buffer input.
void landview_reset_input_buffers();

// We don't have to do much specifically in the land view when we
// start a new turn, but there are a couple of small things to do
// for a polished user experience.
void landview_start_new_turn();

struct Plane;
Plane* land_view_plane();

} // namespace rn
