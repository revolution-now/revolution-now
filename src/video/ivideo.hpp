/****************************************************************
**ivideo.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-27.
*
* Description: Interface for dealing with the program window.
*
*****************************************************************/
#pragma once

// rds
#include "ivideo.rds.hpp"

// gfx
#include "gfx/monitor.rds.hpp"

// base
#include "base/expect.hpp"

// C++ standard library
#include <source_location>

namespace vid {

struct error {
  std::source_location loc = std::source_location::current();
  std::string msg;

  friend void to_str( error const& o, std::string& out,
                      base::tag<error> );
};

template<typename T>
using or_err = base::expect<T, error>;

/****************************************************************
** IVideo
*****************************************************************/
struct IVideo {
  virtual ~IVideo() = default;

  virtual or_err<DisplayMode> display_mode() = 0;

  virtual or_err<gfx::MonitorDpi> display_dpi() = 0;

  virtual or_err<WindowHandle> create_window(
      WindowOptions const& options ) = 0;

  virtual void destroy_window( WindowHandle const& wh ) = 0;

  virtual void hide_window( WindowHandle const& wh ) = 0;

  virtual void restore_window( WindowHandle const& wh ) = 0;

  virtual bool is_window_fullscreen(
      WindowHandle const& wh ) = 0;

  virtual void set_fullscreen( WindowHandle const& wh,
                               bool fullscreen ) = 0;

  virtual std::string window_title( WindowHandle const& wh ) = 0;

  virtual void set_window_title( WindowHandle const& wh,
                                 std::string const& title ) = 0;

  virtual gfx::size window_size( WindowHandle const& wh ) = 0;

  virtual void set_window_size( WindowHandle const& wh,
                                gfx::size sz ) = 0;

  virtual RenderingBackendContext
  init_window_for_rendering_backend(
      WindowHandle const& wh,
      RenderingBackendOptions const& options ) = 0;

  virtual void free_rendering_backend_context(
      RenderingBackendContext const& context ) = 0;

  virtual void swap_double_buffer( WindowHandle const& wh ) = 0;
};

} // namespace vid
