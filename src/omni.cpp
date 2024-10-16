/****************************************************************
**omni.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-08.
*
* Description: A plane that is always on the top of the stack.
*
*****************************************************************/
#include "omni.hpp"

// Revolution Now
#include "frame.hpp"
#include "input.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "tiles.hpp"

// config
#include "config/tile-enum.rds.hpp"

// luapp
#include "luapp/register.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/aspect.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

bool g_debug_omni_overlay = true;

string toggle_omni_overlay() {
  g_debug_omni_overlay = !g_debug_omni_overlay;
  return g_debug_omni_overlay ? "on" : "off";
}

} // namespace

/****************************************************************
** OmniPlane::Impl
*****************************************************************/
// This plane is intended to be:
//
//   1) Always present.
//   2) Always (mostly) invisible.
//   3) Always on top.
//   4) Catching any global events (such as special key presses).
//
struct OmniPlane::Impl : public IPlane {
  Impl() = default;

  void render_framerate( rr::Renderer& renderer ) const {
    gfx::point const framerate_se_corner =
        gfx::point{} + renderer.logical_screen_size();
    vector<string> const lines = {
      fmt::format( "fps: {:.1f}", avg_frame_rate() ) };
    static gfx::pixel shaded_wood =
        gfx::pixel::wood().shaded( 2 );
    render_text_overlay_with_anchor(
        renderer, lines, framerate_se_corner, e_cdirection::se,
        gfx::pixel::banana(), shaded_wood );
  }

  void render_aspect_info( rr::Renderer& renderer ) const {
    vector<string> lines;

    static auto line_logger = []( vector<string>& lines
                                      ATTR_LIFETIMEBOUND ) {
      return
          [&]<typename... Args>(
              fmt::format_string<std::type_identity_t<Args>...>
                  fmt_str,
              Args&&... args ) {
            lines.push_back( fmt::format(
                fmt_str, std::forward<Args>( args )... ) );
          };
    };

    auto log = line_logger( lines );

    auto fmt_size = []( gfx::size const s ) {
      return fmt::format( "{}x{}", s.w, s.h );
    };

    auto fmt_point = []( gfx::point const p ) {
      return fmt::format( "[{},{}]", p.x, p.y );
    };

    log( "Aspect Info:" );

    gfx::size const physical_size = main_window_physical_size();
    log( " physical: {}", fmt_size( physical_size ) );

    gfx::size const logical_size =
        renderer.logical_screen_size();
    log( " logical:  {}", fmt_size( logical_size ) );

    UNWRAP_CHECK_T(
        auto const actual_ratio,
        gfx::AspectRatio::from_size( physical_size ) );
    log( " aspect:   {}", actual_ratio );

    auto const closest_named_ratio =
        find_closest_named_aspect_ratio(
            actual_ratio, gfx::default_aspect_ratio_tolerance() )
            .fmap( gfx::named_ratio_canonical_name );
    log( " closest:  {}", closest_named_ratio );

    auto const& resolution = get_resolution();

    log( " resolution:" );
    log( "  scale:   {}", resolution.scale );
    log( "  exact:   {}", resolution.exact );
    log( "  target:  {}",
         fmt_size( resolution.target_logical ) );
    log( "  buffer:  {}", fmt_size( resolution.buffer ) );
    log( "  score:   {}", resolution.score );
    log( "  clipped:", resolution.clipped_logical );
    log( "   origin: {}",
         fmt_point( resolution.clipped_logical.origin ) );
    log( "   size:   {}",
         fmt_size( resolution.clipped_logical.size ) );
    log( "  logical: {}", fmt_size( resolution.logical ) );

    gfx::size const lg_in_ph =
        resolution.logical * resolution.scale;
    gfx::rect const physical_rect{ .origin = {},
                                   .size = resolution.physical };
    auto const      ph_viewport_origin =
        gfx::centered_in( lg_in_ph, physical_rect );
    gfx::rect const ph_viewport_rect{
      .origin = ph_viewport_origin, .size = lg_in_ph };
    log( " ph_viewport:" );
    log( "  nw:      {}", fmt_point( ph_viewport_rect.nw() ) );
    log( "  se:      {}", fmt_point( ph_viewport_rect.se() ) );
    auto const lg_viewport_rect =
        ph_viewport_rect / resolution.scale;
    log( " lg_viewport:" );
    log( "  nw:      {}", fmt_point( lg_viewport_rect.nw() ) );
    log( "  se:      {}", fmt_point( lg_viewport_rect.se() ) );

    // rr::Painter painter = renderer.painter();
    // painter.draw_empty_rect(
    //     gfx::rect{ .origin = {}, .size = resolution.logical },
    //     rr::Painter::e_border_mode::inside, gfx::pixel::red()
    //     );

    gfx::point const info_region_anchor = [&] {
      gfx::point res;
      res = gfx::point{ .x = 50, .y = 50 };
      return res;
    }();

    render_text_overlay_with_anchor(
        renderer, lines, info_region_anchor, e_cdirection::nw,
        gfx::pixel::white(), gfx::pixel::black() );
  }

  void render_debug_overlay( rr::Renderer& renderer ) const {
    render_aspect_info( renderer );
  }

  void draw( rr::Renderer& renderer ) const override {
    render_framerate( renderer );
    if( g_debug_omni_overlay ) render_debug_overlay( renderer );
    render_sprite(
        renderer,
        input::current_mouse_position() - Delta{ .w = 16 },
        e_tile::mouse_arrow1 );
  }

  e_input_handled input( input::event_t const& event ) override {
    auto handled = e_input_handled::no;
    SWITCH( event ) {
      CASE( quit_event ) { throw exception_exit{}; }
      CASE( key_event ) {
        if( key_event.change != input::e_key_change::down )
          break;
        handled = e_input_handled::yes;
        switch( key_event.keycode ) {
          case ::SDLK_F12:
            // if( !screenshot() )
            //   lg.warn( "failed to take screenshot." );
            break;
          case ::SDLK_F11:
            toggle_fullscreen();
            break;
          case ::SDLK_MINUS:
            // if( key_event.mod.ctrl_down )
            //   dec_resolution_scale();
            // else
            handled = e_input_handled::no;
            break;
          case ::SDLK_EQUALS:
            // if( key_event.mod.ctrl_down )
            //   inc_resolution_scale();
            // else
            handled = e_input_handled::no;
            break;
          case ::SDLK_q:
            if( key_event.mod.ctrl_down ) throw exception_exit{};
            handled = e_input_handled::no;
            break;
          default: //
            handled = e_input_handled::no;
            break;
        }
        break;
      }
      default:
        break;
    }
    return handled;
  }
};

/****************************************************************
** OmniPlane
*****************************************************************/
IPlane& OmniPlane::impl() { return *impl_; }

OmniPlane::~OmniPlane() = default;

OmniPlane::OmniPlane() : impl_( new Impl ) {}

/****************************************************************
** Lua
*****************************************************************/
namespace {

LUA_AUTO_FN( toggle_omni_overlay );

}

} // namespace rn
