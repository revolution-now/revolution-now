/****************************************************************
**stencil.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-11.
*
* Description: Common helpers for stencils.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/pixel.hpp"

namespace rr {

struct StencilPlan {
  int replacement_atlas_id = {};
  gfx::pixel key_color     = {};
};

} // namespace rr
