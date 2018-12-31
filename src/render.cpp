/****************************************************************
**render.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Performs all rendering for game.
*
*****************************************************************/
#include "render.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "movement.hpp"
#include "orders.hpp"
#include "ownership.hpp"
#include "plane.hpp"
#include "sdl-util.hpp"
#include "viewport.hpp"
#include "world.hpp"

// base-util
#include "base-util/variant.hpp"

// SDL
#include "SDL.h"

// C++ standard library
#include <vector>

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Rendering Building Blocks
*****************************************************************/
void render_landscape( Texture const& tx, Coord world_square,
                       Coord pixel_coord ) {
  auto   s    = square_at( world_square );
  g_tile tile = s.land ? g_tile::land : g_tile::water;
  render_sprite( tx, tile, pixel_coord, 0, 0 );
}

void render_unit( Texture const& tx, UnitId id,
                  Coord pixel_coord ) {
  auto const& unit = unit_from_id( id );
  render_sprite( tx, unit.desc().tile, pixel_coord, 0, 0 );
}

/****************************************************************
** Viewport Rendering
*****************************************************************/
// TODO: the state should eventually be const when animations are
//       sorted out.
void render_world_viewport( ViewportState& state ) {
  set_render_target( g_texture_viewport );

  auto covered = viewport().covered_tiles();

  Opt<Coord>  blink_coords;
  Opt<UnitId> blink_id;
  // if( util::holds( state, viewport_state::blink_unit
  if_v( state, viewport_state::blink_unit, blink ) {
    blink_coords = coords_for_unit( blink->id );
    blink_id     = blink->id;
  }

  Opt<UnitId> slide_id;
  // if( util::holds( state, viewport_state::blink_unit
  if_v( state, viewport_state::slide_unit, slide ) {
    slide_id = slide->id;
  }

  for( auto coord : covered ) {
    Coord pixel_coord =
        Coord{} + ( coord - covered.upper_left() );
    pixel_coord *= g_tile_scale;

    // First the land.
    render_landscape( g_texture_viewport, coord, pixel_coord );

    bool is_blink_square = ( coord == blink_coords );

    // Next the units.

    // If this square has been requested to be blank OR if there
    // is a blinking unit on it then skip rendering units on it.
    if( !is_blink_square ) {
      // Render all units on this square as usual.
      for( auto id : units_from_coord( coord ) )
        if( slide_id != id )
          render_unit( g_texture_viewport, id, pixel_coord );
    }

    // Now do a blinking unit, if any.
    if( blink_id && is_blink_square ) {
      using namespace std::chrono;
      using namespace std::literals::chrono_literals;
      auto time        = system_clock::now().time_since_epoch();
      auto one_second  = 1000ms;
      auto half_second = 500ms;
      if( time % one_second > half_second )
        render_unit( g_texture_viewport, *blink_id,
                     pixel_coord );
    }
  }

  // Now do sliding, if any
  if_v( state, viewport_state::slide_unit, slide ) {
    slide->percent_vel.advance( e_push_direction::none );
    slide->percent += slide->percent_vel.to_double();
    if( slide->percent > 1.0 ) slide->percent = 1.0;

    Coord coords = coords_for_unit( slide->id );
    Delta delta  = slide->target - coords;
    CHECK( -1 <= delta.w && delta.w <= 1 );
    CHECK( -1 <= delta.h && delta.h <= 1 );
    delta *= g_tile_scale;
    Delta pixel_delta{W( int( delta.w._ * slide->percent ) ),
                      H( int( delta.h._ * slide->percent ) )};

    auto  covered = viewport().covered_tiles();
    Coord pixel_coord =
        Coord{} + ( coords - covered.upper_left() );
    pixel_coord *= g_tile_scale;
    pixel_coord += pixel_delta;
    auto const& unit = unit_from_id( slide->id );
    render_sprite( g_texture_viewport, unit.desc().tile,
                   pixel_coord, 0, 0 );
  }
}

namespace {

ViewportState g_viewport_state{viewport_state::none{}};

struct ViewportPlane : public Plane {
  ViewportPlane() = default;
  bool enabled() const override { return true; }
  bool covers_screen() const override { return true; }
  void draw( Texture const& tx ) const override {
    render_world_viewport( g_viewport_state );
    copy_texture_stretch( g_texture_viewport, tx,
                          viewport().get_render_src_rect(),
                          viewport().get_render_dest_rect() );
  }
  bool input( input::event_t const& event ) override {
    bool handled = false;
    switch_v( event.event ) {
      case_v( input::unknown_event_t ) {}
      case_v( input::quit_event_t ) {}
      case_v( input::key_event_t ) {
        auto& key_event = val;
        if( key_event.change != input::e_key_change::down )
          break_v;
        switch_v( g_viewport_state ) {
          case_v( viewport_state::none ) {}
          case_v( viewport_state::slide_unit ) {}
          case_v( viewport_state::blink_unit ) {
            auto& blink_unit = val;
            switch( key_event.keycode ) {
              case ::SDLK_q:
                // TODO: temporary
                blink_unit.orders = orders::quit;
                handled           = true;
                break;
              case ::SDLK_t:
                blink_unit.orders = orders::wait;
                handled           = true;
                break;
              case ::SDLK_SPACE:
              case ::SDLK_KP_5:
                blink_unit.orders = orders::forfeight;
                handled           = true;
                break;
              default:
                if( key_event.direction ) {
                  blink_unit.orders =
                      orders::move{*key_event.direction};
                  handled = true;
                }
                break;
            }
          }
          default_v;
        }
      }
      case_v( input::mouse_event_t ) {
        if( val.kind == input::e_mouse_event_kind::wheel ) {
          if( val.wheel_delta < 0 )
            viewport().set_zoom_push(
                e_push_direction::negative );
          if( val.wheel_delta > 0 )
            viewport().set_zoom_push(
                e_push_direction::positive );
          // A user zoom request halts any auto zooming that
          // may currently be happening.
          viewport().stop_normalize_zoom();
          handled = true;
        }
      }
      default_v;
    }
    return handled;
  }
};

ViewportPlane g_viewport_plane;

} // namespace

ViewportState& viewport_rendering_state() {
  return g_viewport_state;
}

Plane* viewport_plane() { return &g_viewport_plane; }

/****************************************************************
** Panel Rendering
*****************************************************************/
namespace {

struct PanelPlane : public Plane {
  PanelPlane() = default;
  bool enabled() const override { return true; }
  bool covers_screen() const override { return false; }
  void draw( Texture const& tx ) const override {
    constexpr int panel_width{6};
    auto          bottom_bar = 0_y + screen_height_tiles() - 1;
    auto left_side = 0_x + screen_width_tiles() - panel_width;
    // bottom edge
    for( X i( 0 ); i - 0_x < screen_width_tiles() - panel_width;
         ++i )
      render_sprite_grid( tx, g_tile::panel_edge_left,
                          bottom_bar, i, 1, 0 );
    // left edge
    for( Y i( 0 ); i - 0_y < screen_height_tiles() - 1; ++i )
      render_sprite_grid( tx, g_tile::panel_edge_left, i,
                          left_side, 0, 0 );
    // bottom left corner of main panel
    render_sprite_grid( tx, g_tile::panel, bottom_bar, left_side,
                        0, 0 );

    for( Y i( 0 ); i - 0_y < screen_height_tiles(); ++i )
      for( X j( left_side + 1 ); j - 0_x < screen_width_tiles();
           ++j )
        render_sprite_grid( tx, g_tile::panel, i, j, 0, 0 );
  }
};

PanelPlane g_panel_plane;

} // namespace

Plane* panel_plane() { return &g_panel_plane; }

namespace {

struct EffectsPlane : public Plane {
  EffectsPlane() = default;
  bool enabled() const override { return enabled_; }
  bool covers_screen() const override { return false; }
  void draw( Texture const& tx ) const override {
    using namespace chrono;
    clear_texture_transparent( tx );
    auto now = system_clock::now();
    if( now < start_time ) return;
    uint8_t alpha = target_alpha;
    if( now < end_time ) {
      milliseconds delta =
          duration_cast<milliseconds>( end_time - now );
      milliseconds total =
          duration_cast<milliseconds>( end_time - start_time );
      double ratio = double( delta.count() ) / total.count();
      CHECK( ratio >= 0.0 && ratio <= 1.0 );
      alpha =
          uint8_t( double( target_alpha ) * ( 1.0 - ratio ) );
    }
    // TODO: this may not require changing alpha to work.
    render_fill_rect( tx, Color( 0, 0, 0, alpha ),
                      screen_logical_rect() );
  }
  TimeType start_time;
  TimeType end_time;
  uint8_t  target_alpha;
  bool     enabled_{false};
};

EffectsPlane g_effects_plane;

} // namespace

Plane* effects_plane() { return &g_effects_plane; }

void effects_plane_enable( bool enable ) {
  g_effects_plane.enabled_ = enable;
}

void reset_fade_to_dark( chrono::milliseconds wait,
                         chrono::milliseconds fade,
                         uint8_t              target_alpha ) {
  CHECK( wait >= 0ms && fade >= 0ms );
  auto now                     = chrono::system_clock::now();
  g_effects_plane.start_time   = now + wait;
  g_effects_plane.end_time     = now + wait + fade;
  g_effects_plane.target_alpha = target_alpha;
}

} // namespace rn
