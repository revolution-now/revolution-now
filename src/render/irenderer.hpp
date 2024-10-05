/****************************************************************
**irenderer.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-04.
*
* Description: Interface for renderer.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/pixel.hpp"

// C++ standard library
#include <vector>

namespace rr {

/****************************************************************
** IRenderer
*****************************************************************/
// This in general only contains a subset of interface methods,
// added as needed.
struct IRenderer {
  virtual ~IRenderer() = default;

  virtual void set_color_cycle_stage( int stage ) = 0;

  virtual void set_color_cycle_plans(
      std::vector<gfx::pixel> const& plans ) = 0;
};

} // namespace rr
