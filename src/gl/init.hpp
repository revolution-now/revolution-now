/****************************************************************
**init.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-12.
*
* Description: Initializes OpenGL given a window/context.
*
*****************************************************************/
#pragma once

// gl
#include "iface.hpp"

// gfx
#include "gfx/cartesian.hpp"

// C++ standard library
#include <memory>
#include <string>

namespace gl {

struct DriverInfo {
  std::string vendor           = {};
  std::string version          = {};
  std::string renderer         = {};
  gfx::size   max_texture_size = {};

  std::string pretty_print() const;
};

struct OpenGLWithLogger;

struct InitResult {
  DriverInfo driver_info = {};

  // This is the interface that should be used to call OpenGL.
  std::unique_ptr<IOpenGL> iface = {};

  // May be null if there is no logging enabled.
  std::unique_ptr<OpenGLWithLogger> logging_iface = {};
};

struct InitOptions {
  bool      enable_glfunc_logging              = false;
  gfx::size initial_window_physical_pixel_size = {};
};

// The window and context must have been created first.
InitResult init_opengl( InitOptions opts = {} );

} // namespace gl
