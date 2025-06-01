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

// rds
#include "irenderer.rds.hpp"

// gfx
#include "gfx/pixel.hpp"

// C++ standard library
#include <span>
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

  virtual void set_color_cycle_keys(
      std::span<gfx::pixel const> plans ) = 0;
};

/****************************************************************
** Interface for changing dynamic renderer settings.
*****************************************************************/
// The idea with this is that it provides a way to access the
// renderer in a safe way to read a write rendering settings that
// doesn't allow doing any rendering (which is only supposed to
// be done from draw() methods which are given the renderer in a
// controlled manner). This interface, on the other can, can be
// used freely.
struct IRendererSettings {
  virtual ~IRendererSettings() = default;

  virtual e_render_framebuffer_mode render_framebuffer_mode()
      const = 0;

  virtual void set_render_framebuffer_mode(
      e_render_framebuffer_mode mode ) = 0;
};

} // namespace rr
