/****************************************************************
**video_sdl.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-27.
*
* Description: Implementation of IVideo using an SDL backend.
*
*****************************************************************/
#pragma once

// video
#include "ivideo.hpp"

namespace vid {

/****************************************************************
** VideoSDL
*****************************************************************/
struct VideoSDL : IVideo {
  or_err<DisplayMode> display_mode() override;

  or_err<gfx::MonitorDpi> display_dpi() override;

  or_err<WindowHandle> create_window(
      WindowOptions const& options ) override;

  void destroy_window( WindowHandle const& wh ) override;

  void hide_window( WindowHandle const& wh ) override;

  void restore_window( WindowHandle const& wh ) override;

  bool is_window_fullscreen( WindowHandle const& wh ) override;

  void set_fullscreen( WindowHandle const& wh,
                       bool fullscreen ) override;

  gfx::size window_size( WindowHandle const& wh ) override;

  void set_window_size( WindowHandle const& wh,
                        gfx::size sz ) override;

  RenderingBackendContext init_window_for_rendering_backend(
      WindowHandle const& wh,
      RenderingBackendOptions const& options ) override;

  void free_rendering_backend_context(
      RenderingBackendContext const& context ) override;

  void swap_double_buffer( WindowHandle const& wh ) override;
};

static_assert( !std::is_abstract_v<VideoSDL> );

} // namespace vid
