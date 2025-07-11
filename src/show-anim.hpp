/****************************************************************
**show-anim.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-29.
*
* Description: Decides whether an event should be animated.
*
*****************************************************************/
#pragma once

// rds
#include "show-anim.rds.hpp"

namespace rn {

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct SSConst;

/****************************************************************
** Public API.
*****************************************************************/
// When the entire animation is contained to a single tile.
bool should_animate_1( SSConst const& ss, gfx::point tile );

// When the animation spans two adjacent tiles.
bool should_animate_2( SSConst const& ss, gfx::point from,
                       gfx::e_direction to );

} // namespace rn
