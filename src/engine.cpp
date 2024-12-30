/****************************************************************
**engine.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-28.
*
* Description: Controller of the main game engine components.
*
*****************************************************************/
#include "engine.hpp"

// Revolution Now
#include "color-cycle.hpp"
#include "logger.hpp"
#include "screen.hpp" // FIXME: remove

// config
#include "config/gfx.rds.hpp"
#include "config/tile-sheet.rds.hpp"

// video
#include "video/video_sdl.hpp"
#include "video/window.hpp"

// render
#include "render/renderer.hpp"

// gl
#include "gl/init.hpp"

// gfx
#include "gfx/resolution.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::maybe;

} // namespace

/****************************************************************
** Engine::Impl
*****************************************************************/
struct Engine::Impl {
  // ============================================================
  // Video
  // ============================================================
  void init_video() {
    CHECK( !video_ );
    video_ = make_unique<vid::VideoSDL>();
  }

  void deinit_video() {
    if( !video_ ) return;
    video_.reset();
  }

  vid::IVideo& video() const {
    CHECK( video_ );
    return *video_;
  }

  // ============================================================
  // Resolutions
  // ============================================================
  void init_resolutions() {
    UNWRAP_CHECK_T( auto const display_mode,
                    video().display_mode() );
    gfx::size const physical_screen = display_mode.size;
    gfx::Monitor const monitor      = monitor_properties(
        physical_screen, monitor_dpi( video() ) );
    resolutions_ = compute_resolutions(
        monitor,
        main_window_physical_size( video(), window() ) );
    CHECK( resolutions_.has_value() );
  }

  void deinit_resolutions() { resolutions_.reset(); }

  gfx::Resolutions& resolutions() {
    CHECK( resolutions_.has_value() );
    return *resolutions_;
  }

  // ============================================================
  // Renderer
  // ============================================================
  void init_renderer() {
    // =========== SDL Stuff

    gfx::size const logical_screen_size =
        main_window_logical_size( video(), window(),
                                  resolutions() );
    gfx::size const physical_screen_size =
        main_window_physical_size( video(), window() );

    vid::RenderingBackendOptions const options{
      .wait_for_vsync = config_gfx.wait_for_vsync };
    rendering_backend_context_ =
        video().init_window_for_rendering_backend( window(),
                                                   options );
    CHECK( rendering_backend_context_.has_value() );

    // =========== gl/iface

    // The window and context must have been created first.
    //
    // NOTE: although the the OpenGL interface pointer is re-
    // turned here, it is not actually used because it is stored
    // in a global variable within the gl library. Perhaps we
    // want to change that at some point.
    gl_iface_ = gl::init_opengl( gl::InitOptions{
      .include_glfunc_logging             = false,
      .initial_window_physical_pixel_size = physical_screen_size,
    } );
    CHECK( gl_iface_.has_value() );
    lg.info( "{}", gl_iface_->driver_info.pretty_print() );

    // =========== Renderer Config

    rr::RendererConfig renderer_config = {
      .logical_screen_size = logical_screen_size,
      .max_atlas_size      = { .w = 3000, .h = 2000 },
      // These are taken by reference.
      .sprite_sheets  = config_tile_sheet.sheets.sprite_sheets,
      .font_sheets    = config_tile_sheet.sheets.font_sheets,
      .dump_atlas_png = config_tile_sheet.dump_texture_atlas_to,
    };

    // This renderer needs to be released before the SDL context
    // is cleaned up.
    renderer_ = rr::Renderer::create( renderer_config, [this] {
      video().swap_double_buffer( window() );
    } );

    // This sends some uniform data to the GPU that is used by
    // the shader to create color cycling effects for e.g. sea
    // lane, rivers, etc.
    set_color_cycle_plans( *renderer_ );
    set_color_cycle_keys( *renderer_ );

    lg.info( "texture atlas size: {}.",
             renderer_->atlas_img_size() );

    // Needed to compute physical/logical resolution based on the
    // window dimensions and then tell the renderer what they
    // are.
    on_main_window_resized( video(), window(), resolutions(),
                            *renderer_ );
  }

  void deinit_renderer() {
    // These must be done in this order.
    renderer_.reset();
    gl_iface_.reset();
    if( rendering_backend_context_.has_value() )
      video().free_rendering_backend_context(
          *rendering_backend_context_ );
    rendering_backend_context_.reset();
  }

  rr::Renderer& renderer() {
    CHECK( renderer_ );
    return *renderer_;
  }

  // ============================================================
  // Window
  // ============================================================
  void init_window() {
    CHECK( !window_.has_value() );
    UNWRAP_CHECK_T( vid::DisplayMode const display_mode,
                    video().display_mode() );
    vid::WindowOptions options{
      .size = display_mode.size,
      .start_fullscreen =
          config_gfx.program_window.start_in_fullscreen };
    UNWRAP_CHECK_T( vid::WindowHandle const handle,
                    video().create_window( options ) );
    window_ = handle;
  }

  void deinit_window() {
    if( !window_ ) return;
    video().destroy_window( *window_ );
    window_.reset();
  }

  vid::WindowHandle const& window() const {
    CHECK( window_.has_value() );
    return *window_;
  }

  void hide_window_if_visible() {
    if( !window_.has_value() ) return;
    video().hide_window( *window_ );
  }

 private:
  unique_ptr<vid::IVideo> video_;
  maybe<gfx::Resolutions> resolutions_;
  unique_ptr<rr::Renderer> renderer_;
  maybe<vid::WindowHandle> window_;
  maybe<vid::RenderingBackendContext> rendering_backend_context_;
  maybe<gl::InitResult> gl_iface_;
};

/****************************************************************
** Engine
*****************************************************************/
Engine::Engine() : pimpl_( new Impl ) {}

Engine::~Engine() { deinit(); }

Engine::Impl& Engine::impl() {
  CHECK( pimpl_ );
  return *pimpl_;
}

// NOTE: order matters in this method.
void Engine::init( e_engine_mode const mode ) {
  switch( mode ) {
    case e_engine_mode::game: {
      impl().init_video();
      impl().init_window();
      impl().init_resolutions();
      impl().init_renderer();
      break;
    }
    case e_engine_mode::console: {
      NOT_IMPLEMENTED;
    }
    case e_engine_mode::map_editor: {
      NOT_IMPLEMENTED;
    }
    case e_engine_mode::ui_test: {
      NOT_IMPLEMENTED;
    }
  }
}

void Engine::deinit() {
  // Do this first so that the window doesn't linger with weird
  // contents as the other subsystems are deinitializing.
  impl().hide_window_if_visible();

  // Reverse order.
  impl().deinit_renderer();
  impl().deinit_resolutions();
  impl().deinit_window();
  impl().deinit_video();
}

vid::IVideo& Engine::video() { return impl().video(); }

vid::WindowHandle const& Engine::window() {
  return impl().window();
}

rr::Renderer& Engine::renderer_use_only_when_needed() {
  return impl().renderer();
}

gfx::Resolutions& Engine::resolutions() {
  return impl().resolutions();
}

} // namespace rn
