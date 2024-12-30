/****************************************************************
**ivideo.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-30.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// video
#include "src/video/ivideo.hpp"

// mock
#include "src/mock/mock.hpp"

namespace vid {

/****************************************************************
** MockIVideo
*****************************************************************/
struct MockIVideo : IVideo {
  MOCK_METHOD( or_err<DisplayMode>, display_mode, (), () );

  MOCK_METHOD( or_err<gfx::MonitorDpi>, display_dpi, (), () );

  MOCK_METHOD( or_err<WindowHandle>, create_window,
               (WindowOptions const&), () );

  MOCK_METHOD( void, destroy_window, (WindowHandle const&), () );

  MOCK_METHOD( void, hide_window, (WindowHandle const&), () );

  MOCK_METHOD( void, restore_window, (WindowHandle const&), () );

  MOCK_METHOD( bool, is_window_fullscreen, (WindowHandle const&),
               () );

  MOCK_METHOD( void, set_fullscreen, (WindowHandle const&, bool),
               () );

  MOCK_METHOD( gfx::size, window_size, (WindowHandle const&),
               () );

  MOCK_METHOD( void, set_window_size,
               ( WindowHandle const&, gfx::size ), () );

  MOCK_METHOD( RenderingBackendContext,
               init_window_for_rendering_backend,
               (WindowHandle const&,
                RenderingBackendOptions const&),
               () );

  MOCK_METHOD( void, free_rendering_backend_context,
               (RenderingBackendContext const&), () );

  MOCK_METHOD( void, swap_double_buffer, (WindowHandle const&),
               () );
};

static_assert( !std::is_abstract_v<MockIVideo> );

} // namespace vid
