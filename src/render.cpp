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
#include "config-files.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "orders.hpp"
#include "ownership.hpp"
#include "plane.hpp"
#include "rand.hpp"
#include "sdl-util.hpp"
#include "travel.hpp"
#include "viewport.hpp"
#include "world.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/variant.hpp"

// SDL
#include "SDL.h"

// C++ standard library
#include <vector>

using namespace std;

namespace rn {

namespace {

constexpr Delta nationality_icon_size( 13_h, 13_w );

} // namespace

/****************************************************************
** Rendering Building Blocks
*****************************************************************/
// TODO: rendered texture needs to be cached.
void render_nationality_icon( Texture const& tx, e_nation nation,
                              char c, Coord pixel_coord ) {
  Delta       delta    = nationality_icon_size;
  Rect        rect     = Rect::from( pixel_coord, delta );
  auto const& nation_o = nation_obj( nation );

  auto color  = nation_o.flag_color;
  auto dark1  = color.shaded( 2 );
  auto dark2  = dark1.shaded( 2 );
  auto dark3  = dark2.shaded( 2 );
  auto light1 = color.highlighted( 1 );
  auto light2 = light1.highlighted( 1 );
  auto light3 = light2.highlighted( 1 );

  auto text_color = color.shaded( 7 );

  render_fill_rect( tx, color, rect );

  render_line( tx, light1, pixel_coord + 1_w,
               {0_h, delta.w - 1_w} );
  render_line( tx, light1, pixel_coord + ( delta.w - 1_w ),
               {delta.h - 1_h, 0_w} );
  render_line( tx, light2, pixel_coord + 4_w,
               {0_h, delta.w - 4_w} );
  render_line( tx, light2, pixel_coord + ( delta.w - 1_w ),
               {delta.h - 4_h, 0_w} );
  render_line( tx, light3, pixel_coord + 7_w,
               {0_h, delta.w - 7_w} );
  render_line( tx, light3, pixel_coord + ( delta.w - 1_w ),
               {delta.h - 7_h, 0_w} );

  render_line( tx, dark1, pixel_coord + 1_h,
               {delta.h - 1_h, 0_w} );
  render_line( tx, dark1, pixel_coord + ( delta.h - 1_h ),
               {0_h, delta.w - 1_w} );
  render_line( tx, dark2, pixel_coord + 4_h,
               {delta.h - 4_h, 0_w} );
  render_line( tx, dark2, pixel_coord + ( delta.h - 1_h ),
               {0_h, delta.w - 4_w} );
  render_line( tx, dark3, pixel_coord + 7_h,
               {delta.h - 7_h, 0_w} );
  render_line( tx, dark3, pixel_coord + ( delta.h - 1_h ),
               {0_h, delta.w - 7_w} );

  auto char_tx = render_text_line_standard(
      fonts::standard, text_color, string( 1, c ) );

  auto char_tx_size = texture_delta( char_tx );
  copy_texture(
      char_tx, tx,
      centered( char_tx_size, rect ) + Delta{1_w, 0_h} );
}

void render_nationality_icon( Texture const& tx, UnitId id,
                              Coord pixel_coord ) {
  auto const& unit = unit_from_id( id );
  // Now we will advance the pixel_coord to put the icon at the
  // location specified in the unit descriptor.
  auto position = unit.desc().nat_icon_position;
  switch( position ) {
    case +e_direction::nw: break;
    case +e_direction::ne:
      pixel_coord +=
          ( ( 1_w * g_tile_width ) - nationality_icon_size.w );
      break;
    case +e_direction::se:
      pixel_coord += ( ( Delta{1_w, 1_h} * g_tile_scale ) -
                       nationality_icon_size );
      break;
    case +e_direction::sw:
      pixel_coord +=
          ( ( 1_h * g_tile_height ) - nationality_icon_size.h );
      break;
      // By default we keep it in the northwest corner.
    default: break;
  };

  char c;
  switch( unit.orders() ) {
    case Unit::e_orders::none: c = '-'; break;
    case Unit::e_orders::sentry: c = 'S'; break;
    case Unit::e_orders::fortified: c = 'F'; break;
  };
  render_nationality_icon( tx, unit.nation(), c, pixel_coord );
}

void render_landscape( Texture const& tx, Coord world_square,
                       Coord pixel_coord ) {
  auto   s = square_at( world_square );
  g_tile tile =
      s.crust == +e_crust::land ? g_tile::land : g_tile::water;
  render_sprite( tx, tile, pixel_coord, 0, 0 );
}

void render_unit( Texture const& tx, UnitId id,
                  Coord pixel_coord ) {
  auto const& unit = unit_from_id( id );
  // Should the icon be in front of the unit or in back.
  auto front = unit.desc().nat_icon_front;
  if( !front ) {
    render_nationality_icon( tx, id, pixel_coord );
    render_sprite( tx, unit.desc().tile, pixel_coord, 0, 0 );
  } else {
    render_sprite( tx, unit.desc().tile, pixel_coord, 0, 0 );
    render_nationality_icon( tx, id, pixel_coord );
  }
}

/****************************************************************
** Viewport State
*****************************************************************/
namespace viewport_state {

depixelate_unit::depixelate_unit( UnitId id_ ) : id( id_ ) {
  for( X x{0}; x < X{1} * g_tile_width; x++ )
    for( Y y{0}; y < Y{1} * g_tile_height; y++ )
      all_pixels.push_back( {x, y} );
  rng::shuffle( all_pixels );
  tx = create_texture( W{1} * g_tile_width,
                       H{1} * g_tile_height );
  clear_texture_transparent( tx );
  render_unit( tx, id, Coord{} );
}

} // namespace viewport_state

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
  if_v( state, viewport_state::slide_unit, slide ) {
    slide_id = slide->id;
  }

  Opt<UnitId> depixelate_id;
  if_v( state, viewport_state::depixelate_unit, dying ) {
    depixelate_id = dying->id;
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
      // TODO: need to figure out what to render when there are
      //       multiple units on a square.
      for( auto id : units_from_coord( coord ) )
        if( slide_id != id && depixelate_id != id )
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
    render_unit( g_texture_viewport, slide->id, pixel_coord );
  }

  // Now do depixelation, if any.
  if_v( state, viewport_state::depixelate_unit, dying ) {
    if( dying->all_pixels.empty() )
      dying->finished = true;
    else {
      int to_depixelate =
          std::min( config_rn.depixelate_pixels_per_frame,
                    int( dying->all_pixels.size() ) );
      vector<Coord> new_non_pixels;
      for( int i = 0; i < to_depixelate; ++i ) {
        auto next_coord = dying->all_pixels.back();
        dying->all_pixels.pop_back();
        new_non_pixels.push_back( next_coord );
      }
      vector<::SDL_Point> points = util::map(
          []( Coord const& c ) { return to_SDL( c ); },
          new_non_pixels );
      set_render_draw_color( {0, 0, 0, 0} );
      set_render_target( dying->tx );
      ::SDL_SetRenderDrawBlendMode( g_renderer,
                                    ::SDL_BLENDMODE_NONE );
      if( !points.empty() )
        ::SDL_RenderDrawPoints( g_renderer, &points[0],
                                points.size() );
      ::SDL_SetRenderDrawBlendMode( g_renderer,
                                    ::SDL_BLENDMODE_BLEND );
      auto  covered = viewport().covered_tiles();
      Coord coords  = coords_for_unit( dying->id );
      Coord pixel_coord =
          Coord{} + ( coords - covered.upper_left() );
      pixel_coord *= g_tile_scale;
      copy_texture( dying->tx, g_texture_viewport, pixel_coord );
    }
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
                          viewport().rendering_src_rect(),
                          viewport().rendering_dest_rect() );
  }
  bool input( input::event_t const& event ) override {
    bool handled = false;
    switch_v( event ) {
      case_v( input::unknown_event_t ) {}
      case_v( input::quit_event_t ) {}
      case_v( input::key_event_t ) {
        // TODO: Need to put this in the input module.
        auto const* __state = ::SDL_GetKeyboardState( nullptr );
        auto        state   = [__state]( ::SDL_Scancode code ) {
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
          return __state[code] != 0;
        };
        // This is because we need to distinguish uppercase from
        // lowercase.
        if( state( ::SDL_SCANCODE_LSHIFT ) ||
            state( ::SDL_SCANCODE_RSHIFT ) )
          break_v;

        auto& key_event = val;
        if( key_event.change != input::e_key_change::down )
          break_v;
        switch_v( g_viewport_state ) {
          case_v( viewport_state::none ) {}
          case_v( viewport_state::slide_unit ) {}
          case_v( viewport_state::depixelate_unit ) {}
          case_v( viewport_state::blink_unit ) {
            auto& blink_unit = val;
            switch( key_event.keycode ) {
              case ::SDLK_z:
                viewport().smooth_zoom_target( 1.0 );
                break;
              case ::SDLK_q:
                // TODO: temporary
                blink_unit.orders = orders::quit;
                handled           = true;
                break;
              case ::SDLK_w:
                blink_unit.orders = orders::wait;
                handled           = true;
                break;
              case ::SDLK_s:
                blink_unit.orders = orders::sentry;
                handled           = true;
                break;
              case ::SDLK_f:
                blink_unit.orders = orders::fortify;
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
                      orders::direction{*key_event.direction};
                  handled = true;
                }
                break;
            }
          }
          default_v;
        }
      }
      case_v( input::mouse_wheel_event_t ) {
        // If the mouse is in the viewport and its a wheel event
        // then we are in business.
        if( viewport().screen_coord_in_viewport( val.pos ) ) {
          if( val.wheel_delta < 0 )
            viewport().set_zoom_push( e_push_direction::negative,
                                      nullopt );
          if( val.wheel_delta > 0 )
            viewport().set_zoom_push( e_push_direction::positive,
                                      val.pos );
          // A user zoom request halts any auto zooming that may
          // currently be happening.
          viewport().stop_auto_zoom();
          viewport().stop_auto_panning();
          handled = true;
        }
      }
      default_v_no_check;
    }
    return handled;
  }
  Plane::e_accept_drag can_drag( input::e_mouse_button button,
                                 Coord origin ) override {
    if( button == input::e_mouse_button::r &&
        viewport().screen_coord_in_viewport( origin ) )
      return Plane::e_accept_drag::yes;
    return Plane::e_accept_drag::no;
  }
  void on_drag( input::e_mouse_button /*unused*/,
                Coord /*unused*/, Coord prev,
                Coord current ) override {
    viewport().stop_auto_panning();
    // When the mouse drags up, we need to move the viewport
    // center down.
    viewport().pan_by_screen_coords( prev - current );
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
      // TODO: are these casts necessary?
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
