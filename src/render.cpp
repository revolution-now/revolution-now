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
#include "logging.hpp"
#include "orders.hpp"
#include "ownership.hpp"
#include "plane.hpp"
#include "rand.hpp"
#include "screen.hpp"
#include "sdl-util.hpp"
#include "terrain.hpp"
#include "travel.hpp"
#include "viewport.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/variant.hpp"

// SDL
#include "SDL.h"

// Abseil
#include "absl/container/flat_hash_map.h"

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
Texture render_nationality_icon_impl( e_nation nation, char c ) {
  Delta       delta = nationality_icon_size;
  Rect        rect  = Rect::from( Coord{}, delta );
  Coord       origin{};
  auto const& nation_o = nation_obj( nation );

  auto tx = create_texture( delta );

  auto color  = nation_o.flag_color;
  auto dark1  = color.shaded( 2 );
  auto dark2  = dark1.shaded( 2 );
  auto dark3  = dark2.shaded( 2 );
  auto light1 = color.highlighted( 1 );
  auto light2 = light1.highlighted( 1 );
  auto light3 = light2.highlighted( 1 );

  auto text_color = color.shaded( 7 );

  render_fill_rect( tx, color, rect );

  render_line( tx, light1, origin + 1_w, {0_h, delta.w - 1_w} );
  render_line( tx, light1, origin + ( delta.w - 1_w ),
               {delta.h - 1_h, 0_w} );
  render_line( tx, light2, origin + 4_w, {0_h, delta.w - 4_w} );
  render_line( tx, light2, origin + ( delta.w - 1_w ),
               {delta.h - 4_h, 0_w} );
  render_line( tx, light3, origin + 7_w, {0_h, delta.w - 7_w} );
  render_line( tx, light3, origin + ( delta.w - 1_w ),
               {delta.h - 7_h, 0_w} );

  render_line( tx, dark1, origin + 1_h, {delta.h - 1_h, 0_w} );
  render_line( tx, dark1, origin + ( delta.h - 1_h ),
               {0_h, delta.w - 1_w} );
  render_line( tx, dark2, origin + 4_h, {delta.h - 4_h, 0_w} );
  render_line( tx, dark2, origin + ( delta.h - 1_h ),
               {0_h, delta.w - 4_w} );
  render_line( tx, dark3, origin + 7_h, {delta.h - 7_h, 0_w} );
  render_line( tx, dark3, origin + ( delta.h - 1_h ),
               {0_h, delta.w - 7_w} );

  auto char_tx = render_text_line_solid(
      fonts::standard, text_color, string( 1, c ) );

  auto char_tx_size = texture_delta( char_tx );
  copy_texture(
      char_tx, tx,
      centered( char_tx_size, rect ) + Delta{1_w, 0_h} );

  return tx;
}

struct NatIconRenderDesc {
  e_nation nation;
  char     c;

  auto to_tuple() const { return tuple{nation, c}; }

  // Abseil hashing API.
  template<typename H>
  friend H AbslHashValue( H h, NatIconRenderDesc const& c ) {
    return H::combine( std::move( h ), c.to_tuple() );
  }

  friend bool operator==( NatIconRenderDesc const& lhs,
                          NatIconRenderDesc const& rhs ) {
    return lhs.to_tuple() == rhs.to_tuple();
  }
};

absl::flat_hash_map<NatIconRenderDesc, Texture> nat_icon_cache;

Texture render_nationality_icon( e_nation nation, char c ) {
  auto do_render = [&] {
    return render_nationality_icon_impl( nation, c );
  };

  NatIconRenderDesc desc{nation, c};

  if( auto maybe_cached =
          util::get_val_safe( nat_icon_cache, desc );
      maybe_cached.has_value() )
    return maybe_cached.value().get().weak_ref();

  nat_icon_cache.emplace( desc, do_render() );
  return nat_icon_cache[desc].weak_ref();
}

void render_nationality_icon( Texture const& dest, UnitId id,
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

  char c{'-'}; // gcc seems to want us to initialize this
  switch( unit.orders() ) {
    case Unit::e_orders::none: c = '-'; break;
    case Unit::e_orders::sentry: c = 'S'; break;
    case Unit::e_orders::fortified: c = 'F'; break;
  };
  auto nat_icon = render_nationality_icon( unit.nation(), c );
  copy_texture( nat_icon, dest, pixel_coord );
}

// Unit only, no nationality icon.
void render_unit( Texture const& tx, e_unit_type unit_type,
                  Coord pixel_coord ) {
  auto const& desc = unit_desc( unit_type );
  render_sprite( tx, desc.tile, pixel_coord, 0, 0 );
}

void render_unit_with_icon( Texture const& tx, UnitId id,
                            Coord pixel_coord ) {
  auto const& unit = unit_from_id( id );
  // Should the icon be in front of the unit or in back.
  if( !unit.desc().nat_icon_front ) {
    render_nationality_icon( tx, id, pixel_coord );
    render_unit( tx, unit.desc().type, pixel_coord );
  } else {
    render_unit( tx, unit.desc().type, pixel_coord );
    render_nationality_icon( tx, id, pixel_coord );
  }
}

/****************************************************************
** Viewport State
*****************************************************************/
namespace viewport_state {

depixelate_unit::depixelate_unit( UnitId           id_,
                                  Opt<e_unit_type> demote_to_ )
  : id( id_ ), demote_to( demote_to_ ) {
  all_pixels.assign( g_tile_rect.begin(), g_tile_rect.end() );
  rng::shuffle( all_pixels );
  tx_from = create_texture( g_tile_delta );
  clear_texture_transparent( tx_from );
  render_unit_with_icon( tx_from, id, Coord{} );

  // Now, if we are depixelating to another unit then we will set
  // that process up.
  if( demote_to.has_value() ) {
    auto tx = create_texture( g_tile_delta );
    clear_texture_transparent( tx );
    auto& unit = unit_from_id( id );
    // Render the target unit with no nationality icon, then
    // render the nationality icon of the unit being depixelated
    // so that it doesn't appear to change during the animation.
    if( !unit.desc().nat_icon_front ) {
      render_unit( tx, *demote_to, Coord{} );
      render_nationality_icon( tx, id, Coord{} );
    } else {
      render_nationality_icon( tx, id, Coord{} );
      render_unit( tx, *demote_to, Coord{} );
    }
    demote_pixels = texture_pixels( tx );
  }
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

  render_terrain( covered, g_texture_viewport, Coord{} );

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
          render_unit_with_icon( g_texture_viewport, id,
                                 pixel_coord );
    }

    // Now do a blinking unit, if any.
    if( blink_id && is_blink_square ) {
      using namespace std::chrono;
      using namespace std::literals::chrono_literals;
      auto time        = system_clock::now().time_since_epoch();
      auto one_second  = 1000ms;
      auto half_second = 500ms;
      if( time % one_second > half_second )
        render_unit_with_icon( g_texture_viewport, *blink_id,
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
    render_unit_with_icon( g_texture_viewport, slide->id,
                           pixel_coord );
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
      ::SDL_SetRenderDrawBlendMode( g_renderer,
                                    ::SDL_BLENDMODE_NONE );
      for( auto point : new_non_pixels ) {
        auto color = dying->demote_pixels.has_value()
                         ? ( *dying->demote_pixels )[point]
                         : Color( 0, 0, 0, 0 );
        set_render_draw_color( color );
        set_render_target( dying->tx_from );
        ::SDL_RenderDrawPoint( g_renderer, point.x._,
                               point.y._ );
      }
      ::SDL_SetRenderDrawBlendMode( g_renderer,
                                    ::SDL_BLENDMODE_BLEND );
      auto  covered = viewport().covered_tiles();
      Coord coords  = coords_for_unit( dying->id );
      Coord pixel_coord =
          Coord{} + ( coords - covered.upper_left() );
      pixel_coord *= g_tile_scale;
      copy_texture( dying->tx_from, g_texture_viewport,
                    pixel_coord );
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
    clear_texture_black( tx );
    copy_texture_stretch(
        g_texture_viewport, tx, viewport().rendering_src_rect(),
        viewport().rendering_dest_rect() + Delta{0_w, 16_h} );
  }
  OptRef<Plane::MenuClickHandler> menu_click_handler(
      e_menu_item item ) const override {
    // These are factors by which the zoom will be scaled when
    // zooming in/out with the menus.
    double constexpr zoom_in_factor  = 2.0;
    double constexpr zoom_out_factor = 1.0 / zoom_in_factor;
    // This is so that a zoom-in followed by a zoom-out will re-
    // store to previous state.
    static_assert( zoom_in_factor * zoom_out_factor == 1.0 );
    if( item == e_menu_item::zoom_in ) {
      static Plane::MenuClickHandler handler = [] {
        // A user zoom request halts any auto zooming that may
        // currently be happening.
        viewport().stop_auto_zoom();
        viewport().stop_auto_panning();
        viewport().smooth_zoom_target( viewport().get_zoom() *
                                       zoom_in_factor );
      };
      return handler;
    }
    if( item == e_menu_item::zoom_out ) {
      static Plane::MenuClickHandler handler = [] {
        // A user zoom request halts any auto zooming that may
        // currently be happening.
        viewport().stop_auto_zoom();
        viewport().stop_auto_panning();
        viewport().smooth_zoom_target( viewport().get_zoom() *
                                       zoom_out_factor );
      };
      return handler;
    }
    if( item == e_menu_item::restore_zoom ) {
      if( viewport().get_zoom() == 1.0 ) return nullopt;
      static Plane::MenuClickHandler handler = [] {
        viewport().smooth_zoom_target( 1.0 );
      };
      return handler;
    }
    return nullopt;
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
    constexpr W panel_width{6 * 32};
    auto left_side = 0_x + screen_logical_size().w - panel_width;

    for( Y i( 16 ); i - 0_y < screen_logical_size().h;
         i += 64_h )
      render_sprite( tx, g_tile::wood_middle, i,
                     left_side + 128_w, 0, 0 );
    for( Y i( 16 ); i - 0_y < screen_logical_size().h;
         i += 64_h )
      render_sprite( tx, g_tile::wood_left_edge, i, left_side, 0,
                     0 );
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
  Time_t  start_time;
  Time_t  end_time;
  uint8_t target_alpha;
  bool    enabled_{false};
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
