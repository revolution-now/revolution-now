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
bool should_animate_event( SSConst const& ss,
                           AnimatedOnTile const& event );

} // namespace rn
