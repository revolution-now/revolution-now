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
#include "conductor.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "midiplayer.hpp"
#include "midiseq.hpp"
#include "oggplayer.hpp"
#include "screen.hpp"
#include "tiles.hpp"

// config
#include "config/debug.rds.hpp"
#include "config/gfx.rds.hpp"
#include "config/rn.rds.hpp"
#include "config/tile-sheet.rds.hpp"

// video
#include "video/video-sdl.hpp"
#include "video/window.hpp"

// sfx
#include "sfx/sfx-sdl.hpp"

// render
#include "render/extra.hpp"
#include "render/renderer.hpp"
#include "render/textometer.hpp"

// Rcl
#include "rcl/model.hpp"
#include "rcl/parse.hpp"

// gl
#include "gl/init.hpp"

// sdl
#include "sdl/init.hpp"

// gfx
#include "gfx/monitor.hpp"
#include "gfx/resolution.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::maybe;
using ::gfx::e_cdirection;
using ::gfx::oriented_point;
using ::gfx::pixel;
using ::gfx::point;

string config_file_for_name( string const& name ) {
  return "config/rcl/" + name + ".rcl";
}

} // namespace

/****************************************************************
** Engine::Impl
*****************************************************************/
struct Engine::Impl {
  // ============================================================
  // Configs.
  // ============================================================
  void init_configs() {
    rds::PopulatorsMap const& populators =
        rds::config_populators();
    // FIXME: we need a mechanism for detecting if we are missing
    // any populators, which can happen if a module is silently
    // dropped by the linker. This can only happen in the unit
    // test binary, but it seems like a good idea to ensure that
    // said binary loads all config files, even if it doesn't use
    // them.
    for( auto const& [name, populator] : populators ) {
      string file = config_file_for_name( name );
      replace( file.begin(), file.end(), '_', '-' );
      base::expect<rcl::doc> doc = rcl::parse_file( file );
      CHECK( doc, "failed to load {}: {}", file, doc.error() );
      lg.debug( "running config populator for {}.", name );
      CHECK_HAS_VALUE( populator( doc->top_val() ) );
    }
  }

  void deinit_configs() {
    // TODO
  }

  // ============================================================
  // SDL Base
  // ============================================================
  void init_sdl_base() { sdl::init_sdl_base(); }

  void deinit_sdl_base() { sdl::deinit_sdl_base(); }

  // ============================================================
  // Input
  // ============================================================
  void init_input() {
    // TODO: temporary until we get a proper interface for key-
    // board and mouse input.
    input::clear_event_queue();
  }

  void deinit_input() {
    // TODO: temporary until we get a proper interface for key-
    // board and mouse input.
    input::clear_event_queue();
  }

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
  // Sound Effects
  // ============================================================
  void init_sfx() {
    CHECK( !sfx_ );
    sfx_ = make_unique<sfx::SfxSDL>();

    sfx_->init_mixer();
    sfx_->load_all_sfx();
  }

  void deinit_sfx() {
    if( !sfx_ ) return;
    // Reverse order.
    sfx_->free_all_sfx();
    sfx_->deinit_mixer();
    sfx_.reset();
  }

  sfx::ISfx& sfx() const {
    CHECK( sfx_ );
    return *sfx_;
  }

  // ============================================================
  // Resolutions
  // ============================================================
  void init_resolutions() {
    UNWRAP_CHECK_T( auto const display_mode,
                    video().display_mode() );
    gfx::size const physical_screen = display_mode.size;
    gfx::Monitor const monitor      = gfx::monitor_properties(
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
      .dump_atlas_png = config_debug.dump.dump_texture_atlas_to,
      // TODO: this should be settable/overridable in the per in-
      // stallation config.
      .framebuffer_mode = config_gfx.post_processing
                              .default_render_framebuffer_mode };

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
    // are. Indications are that this will get called anyway at
    // initialization, potentially due to window events getting
    // injected by SDL due to window startup (which then triggers
    // this function call after the input loop begins), but not
    // 100% sure about that. Either way, this guarantees that it
    // gets called.
    on_main_window_resized( video(), window() );
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
  // Sprite caches.
  // ============================================================
  void init_sprites() {
    rn::init_sprites( renderer() );
    CHECK_HAS_VALUE( validate_sprites( renderer() ) );
  }

  void deinit_sprites() { rn::deinit_sprites(); }

  // ============================================================
  // Textometer.
  // ============================================================
  void init_textometer() {
    CHECK( renderer_ );
    textometer_ = make_unique<rr::Textometer>(
        renderer_->atlas(), renderer_->ascii_font( "simple" ) );
  }

  void deinit_textometer() { textometer_.reset(); }

  rr::ITextometer& textometer() {
    CHECK( textometer_ );
    return *textometer_;
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
          config_gfx.program_window.start_in_fullscreen,
      .title = config_rn.main_window.title };
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

  // ============================================================
  // MIDI Sequencer
  // ============================================================
  void init_midiseq() { midiseq::init_midiseq(); }

  void deinit_midiseq() { midiseq::cleanup_midiseq(); }

  // ============================================================
  // OGG Music Player
  // ============================================================
  void init_oggplayer() { rn::init_oggplayer(); }

  void deinit_oggplayer() { rn::cleanup_oggplayer(); }

  // ============================================================
  // MIDI Music Player
  // ============================================================
  void init_midiplayer() { rn::init_midiplayer(); }

  void deinit_midiplayer() { rn::cleanup_midiplayer(); }

  // ============================================================
  // Conductor
  // ============================================================
  void init_conductor() { conductor::init_conductor(); }

  void deinit_conductor() { conductor::cleanup_conductor(); }

  // ============================================================
  // Miscellaneous Stuff.
  // ============================================================

 public:
  void draw_pause( rr::Renderer& renderer ) const {
    renderer.clear_screen( pixel::black() );
    SCOPED_RENDERER_MOD_SET( buffer_mods.buffer,
                             rr::e_render_buffer::normal );
    point const center = renderer.logical_screen_rect().center();
    rr::render_text_line_with_background(
        renderer, rr::TextLayout{},
        "Game PAUSED - Press the Pause key to resume.",
        oriented_point{ .anchor    = center,
                        .placement = e_cdirection::c },
        pixel::banana(), pixel::black(),
        /*padding=*/4,
        /*draw_corners=*/false );
  }

  void pause() const {
    if( !video_ || !window_ || !renderer_ ) {
      lg.error(
          "cannot pause since engine is not fully "
          "initialized." );
      return;
    }
    auto const old_title = video_->window_title( *window_ );
    SCOPE_EXIT {
      video_->set_window_title( *window_, old_title );
    };
    video_->set_window_title(
        *window_, format( "{} - PAUSED", old_title ) );
    // Don't clear buffers so that everything that is currently
    // on the screen continues to be, that way it looks like our
    // message is just floating on whatever the player was al-
    // ready looking at. WARNING: normal code should never set
    // clear_buffers to false; this is special code.
    renderer_->render_pass(
        [&]( rr::Renderer& renderer ) {
          draw_pause( renderer );
        },
        rr::RenderPassOpts{ .clear_buffers = false } );
    // After the pause is finished the cursor state should be re-
    // stored to whatever it needs to be by the `omni` plane once
    // the player starts moving the mouse again.
    input::set_show_system_cursor( true );
    lg.warn( "entering blocking wait for pause key..." );
    input::blocking_wait_for_key( ::SDLK_PAUSE );
    lg.info( "unpaused." );
  }

 private:
  unique_ptr<vid::IVideo> video_;
  maybe<gfx::Resolutions> resolutions_;
  unique_ptr<rr::Renderer> renderer_;
  maybe<vid::WindowHandle> window_;
  maybe<vid::RenderingBackendContext> rendering_backend_context_;
  maybe<gl::InitResult> gl_iface_;
  unique_ptr<sfx::SfxSDL> sfx_;
  unique_ptr<rr::Textometer> textometer_;
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
  lg.info( "initializing game engine for mode: {}", mode );
  switch( mode ) {
    case e_engine_mode::game: {
      impl().init_configs();
      impl().init_sdl_base();
      impl().init_input();
      impl().init_video();
      impl().init_window();
      impl().init_resolutions();
      impl().init_renderer();
      impl().init_sprites();
      impl().init_textometer();
      impl().init_sfx();
      impl().init_midiseq();
      impl().init_oggplayer();
      impl().init_midiplayer();
      impl().init_conductor();
      break;
    }
    case e_engine_mode::unit_tests: {
      impl().init_configs();
      break;
    }
    case e_engine_mode::console: {
      impl().init_configs();
      break;
    }
    case e_engine_mode::map_editor: {
      impl().init_configs();
      impl().init_sdl_base();
      impl().init_input();
      impl().init_video();
      impl().init_window();
      impl().init_resolutions();
      impl().init_renderer();
      impl().init_sprites();
      impl().init_textometer();
      break;
    }
    case e_engine_mode::ui_test: {
      impl().init_configs();
      impl().init_sdl_base();
      impl().init_input();
      impl().init_video();
      impl().init_window();
      impl().init_resolutions();
      impl().init_renderer();
      impl().init_sprites();
      impl().init_textometer();
      break;
    }
  }
}

void Engine::deinit() {
  lg.info( "closing down game engine." );
  // Do this first so that the window doesn't linger with weird
  // contents as the other subsystems are deinitializing.
  impl().hide_window_if_visible();

  // Reverse order.
  impl().deinit_conductor();
  impl().deinit_midiplayer();
  impl().deinit_oggplayer();
  impl().deinit_midiseq();
  impl().deinit_sfx();
  impl().deinit_textometer();
  impl().deinit_sprites();
  impl().deinit_renderer();
  impl().deinit_resolutions();
  impl().deinit_window();
  impl().deinit_video();
  impl().deinit_input();
  impl().deinit_sdl_base();
  impl().deinit_configs();
}

vid::IVideo& Engine::video() { return impl().video(); }

sfx::ISfx& Engine::sfx() { return impl().sfx(); }

vid::WindowHandle const& Engine::window() {
  return impl().window();
}

rr::Renderer& Engine::renderer_use_only_when_needed() {
  return impl().renderer();
}

rr::IRendererSettings& Engine::renderer_settings() {
  return impl().renderer();
}

gfx::Resolutions& Engine::resolutions() {
  return impl().resolutions();
}

rr::ITextometer& Engine::textometer() {
  return impl().textometer();
}

void Engine::pause() { impl().pause(); }

} // namespace rn
