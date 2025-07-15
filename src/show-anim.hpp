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

// gfx
#include "gfx/cartesian.hpp"

namespace rn {

// NOTE: Most normal game code should not include this module di-
// rectly or call any of its methods, since the decision as to
// whether to carry out an animation is done in the
// land-view-anim module. Most code should animate uncondition-
// ally therefore and let that module decide whether to show it.
// There are a few exceptions to this though.

/****************************************************************
** Fwd Decls.
*****************************************************************/
struct AnimationSequence;
struct SSConst;

/****************************************************************
** Public API.
*****************************************************************/
// This is the preferred way to check for whether we should be
// animating an animation sequence.
bool should_animate_seq( SSConst const& ss,
                         AnimationSequence const& seq );

bool should_animate_tile( SSConst const& ss,
                          AnimatedTile const& anim_tile );

// This is for when you don't have an AnimationSequence object
// and the animation is contained on one tile. The tile must
// exist on the map.
//
// Even if this doesn't end up being used, it should still be ex-
// posed for testing since it contains the core logic.
bool should_animate_tile( SSConst const& ss, gfx::point tile );

} // namespace rn
