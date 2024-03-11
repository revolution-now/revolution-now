/****************************************************************
**hidden-terrain.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-10.
*
* Description: Things related to the "Show Hidden Terrain"
*              feature.
*
*****************************************************************/
#pragma once

// rds
#include "hidden-terrain.rds.hpp"

// Revolution Now
#include "anim-builder.rds.hpp"

namespace rn {

struct IRand;
struct IVisibility;
struct SSConst;

HiddenTerrainAnimationSequence anim_seq_for_hidden_terrain(
    SSConst const& ss, IVisibility const& viz, IRand& rand );

} // namespace rn
