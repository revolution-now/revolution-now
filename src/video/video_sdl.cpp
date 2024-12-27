/****************************************************************
**video_sdl.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-27.
*
* Description: Implementation of IVideo using an SDL backend.
*
*****************************************************************/
#include "video_sdl.hpp"

// video
#include "include-sdl.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/fmt.hpp"

// TODO: replace this with a logging framework.
#define LOG fmt::println

using namespace std;

namespace vid {

namespace {

using ::gfx::rect;
using ::gfx::size;

/****************************************************************
** Constants
*****************************************************************/
constexpr auto kPixelFormat = ::SDL_PIXELFORMAT_RGBA8888;

/****************************************************************
** Helpers
*****************************************************************/
::SDL_Window* handle_for( WindowHandle const& wh ) {
  CHECK( wh.handle != nullptr );
  return static_cast<::SDL_Window*>( wh.handle );
}

// Only call this when the most recent SDL call made signaled an
// error via its return code.
string sdl_get_last_error() { return ::SDL_GetError(); }

void log_video_stats() {
  float ddpi, hdpi, vdpi;
  // A warning from the SDL2 docs:
  //
  //   WARNING: This reports the DPI that the hardware reports,
  //   and it is not always reliable! It is almost always better
  //   to use SDL_GetWindowSize() to find the window size, which
  //   might be in logical points instead of pixels, and then
  //   SDL_GL_GetDrawableSize(), SDL_Vulkan_GetDrawableSize(),
  //   SDL_Metal_GetDrawableSize(), or SDL_Get-Renderer-Output-
  //   Size(), and compare the two values to get an actual
  //   scaling value between the two. We will be rethinking how
  //   high-dpi details should be managed in SDL3 to make things
  //   more consistent, reliable, and clear.
  //
  ::SDL_GetDisplayDPI( 0, &ddpi, &hdpi, &vdpi );
  LOG( "GetDisplayDPI: {{ddpi={}, hdpi={}, vdpi={}}}.", ddpi,
       hdpi, vdpi );

  SDL_DisplayMode dm;

  auto dm_to_str = [&dm] {
    return fmt::format( "{}x{}@{}Hz, pf={}", dm.w, dm.h,
                        dm.refresh_rate,
                        ::SDL_GetPixelFormatName( dm.format ) );
  };
  (void)dm_to_str;

  LOG( "default game pixel format: {}",
       ::SDL_GetPixelFormatName( kPixelFormat ) );

  SDL_GetCurrentDisplayMode( 0, &dm );
  LOG( "GetCurrentDisplayMode: {}", dm_to_str() );

  SDL_GetDesktopDisplayMode( 0, &dm );
  LOG( "GetDesktopDisplayMode: {}", dm_to_str() );

  SDL_GetDisplayMode( 0, 0, &dm );
  LOG( "GetDisplayMode: {}", dm_to_str() );

  SDL_Rect r;
  SDL_GetDisplayBounds( 0, &r );
  LOG( "GetDisplayBounds: {}",
       rect{ .origin = { .x = r.x, .y = r.y },
             .size   = { .w = r.w, .h = r.h } } );
}

} // namespace

/****************************************************************
** VideoSDL
*****************************************************************/
or_err<DisplayMode> VideoSDL::display_mode() {
  SDL_DisplayMode dm;
  if( ::SDL_GetCurrentDisplayMode( 0, &dm ) < 0 ) {
    return error{
      .msg = fmt::format( "failed to get display mode info: {}",
                          sdl_get_last_error() ) };
  }
  return DisplayMode{ .size         = { .w = dm.w, .h = dm.h },
                      .format       = dm.format,
                      .refresh_rate = dm.refresh_rate };
}

or_err<gfx::MonitorDpi> VideoSDL::display_dpi() {
  float hdpi = 0.0; // horizontal dpi.
  float vdpi = 0.0; // vertical dpi.
  float ddpi = 0.0; // diagonal dpi.
  // A warning from the SDL2 docs:
  //
  //   WARNING: This reports the DPI that the hardware reports,
  //   and it is not always reliable! It is almost always
  //   better to use SDL_GetWindowSize() to find the window
  //   size, which might be in logical points instead of pix-
  //   els, and then SDL_GL_GetDrawableSize(), SDL_Vulkan_-Get-
  //   DrawableSize(), SDL_Metal_GetDrawableSize(), or SDL_Get-
  //   RendererOutputSize(), and compare the two values to get
  //   an actual scaling value between the two. We will be re-
  //   thinking how high-dpi details should be managed in SDL3
  //   to make things more consistent, reliable, and clear.
  //
  if( ::SDL_GetDisplayDPI( 0, &ddpi, &hdpi, &vdpi ) < 0 )
    return error{ .msg = sdl_get_last_error() };

  return gfx::MonitorDpi{
    .horizontal = hdpi, .vertical = vdpi, .diagonal = ddpi };
}

or_err<WindowHandle> VideoSDL::create_window(
    WindowOptions const& options ) {
  log_video_stats();

  if( options.size.area() == 0 )
    return error{ .msg =
                      "must specify a size with non-zero area "
                      "when creating a window." };

  int flags = {};

  flags |= ::SDL_WINDOW_SHOWN;
  flags |= ::SDL_WINDOW_OPENGL;
  // Not sure why, but this seems to be beneficial even when we
  // are starting in fullscreen desktop mode, since if it is not
  // set (this is on Linux) then when we leave fullscreen mode
  // the "restore" command doesn't work and the window remains
  // maximized.
  flags |= ::SDL_WINDOW_RESIZABLE;

  if( options.start_fullscreen )
    flags |= ::SDL_WINDOW_FULLSCREEN_DESKTOP;

  auto* const p_window = ::SDL_CreateWindow(
      options.title.c_str(), 0, 0, options.size.w,
      options.size.h, flags );

  if( !p_window )
    return error{ .msg = "failed to create window" };

  return WindowHandle{ .handle = p_window };
}

void VideoSDL::destroy_window( WindowHandle const& wh ) {
  ::SDL_DestroyWindow( handle_for( wh ) );
}

void VideoSDL::hide_window( WindowHandle const& wh ) {
  ::SDL_HideWindow( handle_for( wh ) );
}

void VideoSDL::restore_window( WindowHandle const& wh ) {
  ::SDL_RestoreWindow( handle_for( wh ) );
}

// TODO: mac-os, does not seem to be able to detect when the user
// fullscreens a window.
bool VideoSDL::is_window_fullscreen( WindowHandle const& wh ) {
  // This bit should always be set even if we're in the "desktop"
  // fullscreen mode.
  return ( ::SDL_GetWindowFlags( handle_for( wh ) ) &
           ::SDL_WINDOW_FULLSCREEN ) != 0;
}

void VideoSDL::set_fullscreen( WindowHandle const& wh,
                               bool const fullscreen ) {
  if( fullscreen == is_window_fullscreen( wh ) ) return;

  if( fullscreen ) {
    ::SDL_SetWindowFullscreen( handle_for( wh ),
                               ::SDL_WINDOW_FULLSCREEN_DESKTOP );
  } else {
    ::SDL_SetWindowFullscreen( handle_for( wh ), 0 );
    // This somehow gets erased when we go to fullscreen mode, so
    // it needs to be re-set each time.
    ::SDL_SetWindowResizable( handle_for( wh ),
                              /*resizable=*/::SDL_TRUE );
  }
}

size VideoSDL::window_size( WindowHandle const& wh ) {
  size res;
  ::SDL_GetWindowSize( handle_for( wh ), &res.w, &res.h );
  return res;
}

void VideoSDL::set_window_size( WindowHandle const& wh,
                                gfx::size sz ) {
  ::SDL_SetWindowSize( handle_for( wh ), sz.w, sz.h );
}

} // namespace vid
