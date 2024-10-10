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
    render_framerate( renderer );
    render_sprite(
        renderer,
        input::current_mouse_position() - Delta{ .w = 16 },
        e_tile::mouse_arrow1 );
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
IPlane& OmniPlane::impl() { return *impl_; }

OmniPlane::~OmniPlane() = default;

OmniPlane::OmniPlane() : impl_( new Impl ) {}

} // namespace rn
