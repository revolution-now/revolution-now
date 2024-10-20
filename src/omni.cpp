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

auto line_logger( vector<string>& lines ATTR_LIFETIMEBOUND ) {
  static constexpr auto fmt_type = mp::overload{
    []( gfx::size const s ) -> string {
      return fmt::format( "{}x{}", s.w, s.h );
    },
    []( gfx::point const p ) -> string {
      return fmt::format( "[{},{}]", p.x, p.y );
    },
    []( gfx::rect const r ) -> string {
      return fmt::format( "[{},{}] {}x{}", r.origin.x,
                          r.origin.y, r.size.w, r.size.h );
    },
    []( auto const& o ) -> string { return base::to_str( o ); },
  };
  return
      [&]<typename... Args>( fmt::format_string<Args...> fmt_str,
                             Args const&... args ) {
        // We need the fmt::runtime here because we are changing
        // the type of the args by passing them through fmt_type
        // which would leave them incompatible with the type of
        // the compile-time format string object fmt_str. But we
        // are not losing compile-time format string checking be-
        // cause that has already been done by the construction
        // of fmt_str upon calling this function.
        if constexpr( sizeof...( Args ) == 0 ) {
          (void)fmt_type; // suppress unused variable warning.
        }
        lines.push_back( fmt::format( fmt::runtime( fmt_str ),
                                      fmt_type( args )... ) );
      };
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
    vector<string> lines;
    auto const     log = line_logger( lines );

    log( "f/s: {}", fmt::format( "{:.1f}", avg_frame_rate() ) );

    static gfx::pixel shaded_wood =
        gfx::pixel::wood().shaded( 2 );
    render_text_overlay_with_anchor(
        renderer, lines, renderer.logical_screen_rect().se(),
        e_cdirection::se, gfx::pixel::banana(), shaded_wood );
  }

  void render_bad_window_size_overlay(
      rr::Renderer& renderer ) const {
    auto const  physical_size   = main_window_physical_size();
    rr::Painter painter         = renderer.painter();
    auto const  default_logical = main_window_logical_rect();
    painter.draw_solid_rect( default_logical,
                             gfx::pixel::yellow() );
    vector<string> help_msg{
      fmt::format( "Window size {}x{} not supported.",
                   physical_size.w, physical_size.h ),
      "Please resize your window." };
    render_text_overlay_with_anchor(
        renderer, help_msg, default_logical.center(),
        e_cdirection::c, gfx::pixel::white(),
        gfx::pixel::black() );
  }

  void render_aspect_info( rr::Renderer& renderer ) const {
    auto const resolution = get_resolution();
    if( !resolution.has_value() ) {
      render_bad_window_size_overlay( renderer );
      return;
    }

    vector<string> lines;
    auto const     log = line_logger( lines );

    auto const aspect_approx = [&]( gfx::size const sz ) {
      return gfx::AspectRatio::from_size( sz )
          .bind( gfx::find_closest_named_aspect_ratio )
          .fmap( gfx::named_ratio_canonical_name );
    };

    auto const physical_size   = main_window_physical_size();
    auto const aspect_physical = aspect_approx( physical_size );
    auto const aspect_logical =
        aspect_approx( resolution->logical.dimensions );

    CHECK( resolution.has_value() );

    log( "Resolution:" );
    log( " physical:  {}", physical_size );
    log( " logical:   {}", resolution->logical.dimensions );
    log( " scale:     {}", resolution->logical.scale );
    log( " viewport:  {}", resolution->viewport );
    log( " fit.score: {}", resolution->scores.fitting );
    log( " fit.size:  {}", resolution->scores.size );
    log( " p.aspect: ~{}", aspect_physical );
    log( " p.aspect: ~{}", aspect_logical );

    gfx::point const info_region_anchor =
        gfx::point{ .x = 32, .y = 32 };

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
            if( key_event.mod.ctrl_down )
              cycle_resolution( 1 );
            else
              handled = e_input_handled::no;
            break;
          case ::SDLK_EQUALS:
            if( key_event.mod.ctrl_down )
              cycle_resolution( -1 );
            else
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
