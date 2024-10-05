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

namespace rr {

/****************************************************************
** IRenderer
*****************************************************************/
// This in general only contains a subset of interface methods,
// added as needed.
struct IRenderer {
  virtual ~IRenderer() = default;

  virtual void set_color_cycle_stage( int stage ) = 0;

  virtual int get_color_cycle_span() const = 0;
};

} // namespace rr
