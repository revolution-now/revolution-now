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
#include "plane.hpp"
#include "screen.hpp"
#include "tiles.hpp"

// render
#include "render/renderer.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

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
struct OmniPlane::Impl : public Plane {
  Impl() = default;

  bool covers_screen() const override { return false; }

  void render_framerate( rr::Renderer& renderer ) const {
    rr::Painter painter    = renderer.painter();
    Coord       info_start = Coord::from_gfx(
              gfx::point{} + renderer.logical_screen_size() );
    // Render frame rate.
    string frame_rate =
        fmt::format( "fps: {:.1f}", avg_frame_rate() );
    // TODO: this needs to be cached in a proper way (it's static
    // because it is expensive to shade).
    static gfx::pixel shaded_wood =
        gfx::pixel::wood().shaded( 2 );
    auto delta_for = []( string_view text ) {
      return Delta::from_gfx(
          rr::rendered_text_line_size_pixels( text ) );
    };
    Delta frame_rate_size = delta_for( frame_rate );
    painter.draw_solid_rect(
        gfx::rect{ .origin = info_start - frame_rate_size,
                   .size   = frame_rate_size },
        shaded_wood );
    renderer
        .typer( "simple", info_start - frame_rate_size,
                gfx::pixel::banana() )
        .write( frame_rate );
  }

  void draw( rr::Renderer& renderer ) const override {
    rr::Painter painter = renderer.painter();
    render_framerate( renderer );
    render_sprite( painter, e_tile::mouse_arrow1,
                   input::current_mouse_position() - 16_w );
  }

  e_input_handled input( input::event_t const& event ) override {
    auto handled = e_input_handled::no;
    switch( event.to_enum() ) {
      case input::e_input_event::quit_event:
        throw exception_exit{};
      case input::e_input_event::win_event: //
        break;
      case input::e_input_event::key_event: {
        auto& key_event = event.get<input::key_event_t>();
        if( key_event.change != input::e_key_change::down )
          break;
        handled = e_input_handled::yes;
        switch( key_event.keycode ) {
          case ::SDLK_F12:
            // if( !screenshot() )
            //   lg.warn( "failed to take screenshot." );
            break;
          case ::SDLK_F11:
            if( is_window_fullscreen() ) {
              toggle_fullscreen();
              restore_window();
            } else {
              toggle_fullscreen();
            }
            break;
          case ::SDLK_MINUS:
            if( key_event.mod.ctrl_down )
              dec_resolution_scale();
            else
              handled = e_input_handled::no;
            break;
          case ::SDLK_EQUALS:
            if( key_event.mod.ctrl_down )
              inc_resolution_scale();
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
      default: //
        break;
    }
    return handled;
  }
};

/****************************************************************
** OmniPlane
*****************************************************************/
Plane& OmniPlane::impl() { return *impl_; }

OmniPlane::~OmniPlane() = default;

OmniPlane::OmniPlane() : impl_( new Impl ) {}

} // namespace rn
