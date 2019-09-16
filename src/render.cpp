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
#include "compositor.hpp"
#include "config-files.hpp"
#include "errors.hpp"
#include "gfx.hpp"
#include "logging.hpp"
#include "orders.hpp"
#include "ownership.hpp"
#include "plane.hpp"
#include "rand.hpp"
#include "screen.hpp"
#include "terrain.hpp"
#include "text.hpp"
#include "travel.hpp"
#include "variant.hpp"
#include "viewport.hpp"
#include "views.hpp"
#include "window.hpp"

// Revolution Now (config)
#include "../config/ucl/rn.inl"

// base-util
#include "base-util/algo.hpp"
#include "base-util/keyval.hpp"
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

// Unit only, no nationality icon.
void render_unit_no_icon( Texture& tx, e_unit_type unit_type,
                          Coord pixel_coord ) {
  auto const& desc = unit_desc( unit_type );
  render_sprite( tx, desc.tile, pixel_coord, 0, 0 );
}

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

  auto const& char_tx = render_text(
      font::nat_icon(), text_color, string( 1, c ) );

  auto char_tx_size = char_tx.size();
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

Texture const& render_nationality_icon( e_nation nation,
                                        char     c ) {
  auto do_render = [&] {
    return render_nationality_icon_impl( nation, c );
  };

  NatIconRenderDesc desc{nation, c};

  if( auto maybe_cached = bu::val_safe( nat_icon_cache, desc );
      maybe_cached.has_value() )
    return maybe_cached.value().get();

  nat_icon_cache.emplace( desc, do_render() );
  return nat_icon_cache[desc];
}

void render_nationality_icon( Texture&              dest,
                              UnitDescriptor const& desc,
                              e_nation              nation,
                              e_unit_orders         orders,
                              Coord pixel_coord ) {
  // Now we will advance the pixel_coord to put the icon at the
  // location specified in the unit descriptor.
  auto position = desc.nat_icon_position;
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
  switch( orders ) {
    case +e_unit_orders::none: c = '-'; break;
    case +e_unit_orders::sentry: c = 'S'; break;
    case +e_unit_orders::fortified: c = 'F'; break;
  };
  auto const& nat_icon = render_nationality_icon( nation, c );
  copy_texture( nat_icon, dest, pixel_coord );
}

void render_nationality_icon( Texture& dest, e_unit_type type,
                              e_nation      nation,
                              e_unit_orders orders,
                              Coord         pixel_coord ) {
  render_nationality_icon( dest, unit_desc( type ), nation,
                           orders, pixel_coord );
}

void render_nationality_icon( Texture& dest, UnitId id,
                              Coord pixel_coord ) {
  auto const& unit = unit_from_id( id );
  render_nationality_icon( dest, unit.desc(), unit.nation(),
                           unit.orders(), pixel_coord );
}

void render_unit( Texture& tx, UnitId id, Coord pixel_coord,
                  bool with_icon ) {
  auto const& unit = unit_from_id( id );
  if( with_icon ) {
    // Should the icon be in front of the unit or in back.
    if( !unit.desc().nat_icon_front ) {
      render_nationality_icon( tx, id, pixel_coord );
      render_unit_no_icon( tx, unit.desc().type, pixel_coord );
    } else {
      render_unit_no_icon( tx, unit.desc().type, pixel_coord );
      render_nationality_icon( tx, id, pixel_coord );
    }
  } else {
    render_unit_no_icon( tx, unit.desc().type, pixel_coord );
  }
}

void render_unit( Texture& tx, e_unit_type unit_type,
                  Coord pixel_coord ) {
  render_unit_no_icon( tx, unit_type, pixel_coord );
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
  render_unit( tx_from, id, Coord{}, /*with_icon=*/true );

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
    demote_pixels = tx.pixels();
  }
}

} // namespace viewport_state

/****************************************************************
** Viewport Rendering
*****************************************************************/
// TODO: the state should eventually be const when animations are
//       sorted out.
void render_world_viewport( ViewportState& state ) {
  g_texture_viewport.set_render_target();

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
          render_unit( g_texture_viewport, id, pixel_coord,
                       /*with_icon=*/true );
    }

    // Now do a blinking unit, if any.
    if( blink_id && is_blink_square ) {
      using namespace std::chrono;
      using namespace std::literals::chrono_literals;
      auto time        = system_clock::now().time_since_epoch();
      auto one_second  = 1000ms;
      auto half_second = 500ms;
      if( time % one_second > half_second )
        render_unit( g_texture_viewport, *blink_id, pixel_coord,
                     /*with_icon=*/true );
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
    render_unit( g_texture_viewport, slide->id, pixel_coord,
                 /*with_icon=*/true );
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
        dying->tx_from.set_render_target();
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

// If this is default constructed then it should represent "no
// actions need to be taken."
struct ClickTileActions {
  Vec<UnitId> bring_to_front{};
  Vec<UnitId> add_to_back{};
};

// This gets called when the player clicks on a square containing
// units and will clear their orders and/or activate them. The
// allow_activate flag must be true in order for any units to ei-
// ther be prioritized (meaning that the current blinking unit is
// pre-empted by the new one) or added to the back of the queue
// (so that they will move later in the turn). This is because
// the allow_activate=false is used during end of turn when it
// doesn't make sense to do any manipulation of the queue.
ClickTileActions click_on_world_tile_impl(
    Coord coord, bool allow_activate ) {
  ClickTileActions result{};
  lg.debug( "clicked on tile {}, allow_activate={}", coord,
            allow_activate );
  auto const& units = units_from_coord_recursive( coord );
  if( units.size() == 0 ) {
    lg.debug( "no units on square." );
    return result;
  }
  if( units.size() == 1 ) {
    auto id = *units.begin();
    lg.debug( "unit on square: {}", debug_string( id ) );
    auto& unit = unit_from_id( id );
    if( unit.orders() == e_unit_orders::none ) {
      if( allow_activate ) {
        lg.debug( "activating." );
        result.bring_to_front.push_back( id );
        return result;
      } else {
        // No action.
        lg.debug( "no action." );
        return result;
      }
    } else {
      lg.debug( "clearing orders." );
      unit.clear_orders();
      if( allow_activate ) result.add_to_back.push_back( id );
      return result;
    }
  } else {
    auto selections =
        ui::unit_selection_box( units, allow_activate );
    for( auto const& selection : selections ) {
      auto& sel_unit = unit_from_id( selection.id );
      switch( selection.what ) {
        case +ui::e_unit_selection::clear_orders:
          lg.debug( "clearing orders for {}.",
                    debug_string( sel_unit ) );
          sel_unit.clear_orders();
          if( allow_activate )
            result.add_to_back.push_back( selection.id );
          break;
        case +ui::e_unit_selection::activate:
          CHECK( allow_activate );
          lg.debug( "activating {}.", debug_string( sel_unit ) );
          // Activation implies also to clear orders if they're
          // not already cleared.
          sel_unit.clear_orders();
          result.bring_to_front.push_back( selection.id );
          break;
      }
    }
    return result;
  }
}

// This function will handle all the actions that can happen as a
// result of the player "clicking" on a world tile. This can in-
// clude activiting units, popping up windows, etc.
ClickTileActions click_on_world_tile( Coord coord ) {
  using Res_t = ClickTileActions;
  return match<Res_t>(
      g_viewport_state,
      []( viewport_state::slide_unit const& ) {
        return Res_t{};
      },
      []( viewport_state::depixelate_unit const& ) {
        return Res_t{};
      },
      // During end of turn.
      [&]( viewport_state::none const& ) {
        return click_on_world_tile_impl(
            coord, /*allow_activate=*/false );
      },
      // During unit blinking.
      [&]( viewport_state::blink_unit const& ) {
        return click_on_world_tile_impl(
            coord, /*allow_activate=*/true );
      } );
}

struct ViewportPlane : public Plane {
  ViewportPlane() = default;
  bool enabled() const override { return true; }
  bool covers_screen() const override { return true; }
  void draw( Texture& tx ) const override {
    render_world_viewport( g_viewport_state );
    clear_texture_black( tx );
    copy_texture_stretch( g_texture_viewport, tx,
                          viewport().rendering_src_rect(),
                          viewport().rendering_dest_rect() );
  }
  Opt<Plane::MenuClickHandler> menu_click_handler(
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
    switch_( event ) {
      case_( input::unknown_event_t ) {}
      case_( input::quit_event_t ) {}
      case_( input::key_event_t ) {
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
          break_;

        auto& key_event = val;
        if( key_event.change != input::e_key_change::down )
          break_;
        switch_( g_viewport_state ) {
          case_( viewport_state::none ) {
            switch( key_event.keycode ) {
              case ::SDLK_z:
                viewport().smooth_zoom_target( 1.0 );
                break;
              default: break;
            }
          }
          case_( viewport_state::slide_unit ) {}
          case_( viewport_state::depixelate_unit ) {}
          case_( viewport_state::blink_unit ) {
            auto& blink_unit = val;
            handled          = true;
            switch( key_event.keycode ) {
              case ::SDLK_z:
                viewport().smooth_zoom_target( 1.0 );
                break;
              case ::SDLK_q:
                // TODO: temporary
                blink_unit.orders = orders::quit{};
                break;
              case ::SDLK_w:
                blink_unit.orders = orders::wait{};
                break;
              case ::SDLK_s:
                blink_unit.orders = orders::sentry{};
                break;
              case ::SDLK_f:
                blink_unit.orders = orders::fortify{};
                break;
              case ::SDLK_c:
                viewport().ensure_tile_visible(
                    coords_for_unit( blink_unit.id ),
                    /*smooth=*/true );
                break;
              case ::SDLK_d:
                blink_unit.orders = orders::disband{};
                break;
              case ::SDLK_SPACE:
              case ::SDLK_KP_5:
                blink_unit.orders = orders::forfeight{};
                break;
              default:
                handled = false;
                if( key_event.direction ) {
                  blink_unit.orders =
                      orders::direction{*key_event.direction};
                  handled = true;
                }
                break;
            }
          }
          switch_exhaustive;
        }
      }
      case_( input::mouse_wheel_event_t ) {
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
      case_( input::mouse_button_event_t ) {
        if( val.buttons != input::e_mouse_button_event::left_up )
          break_;
        auto maybe_tile =
            viewport().screen_pixel_to_world_tile( val.pos );
        // See if the cursor has clicked on a tile in the
        // viewport.
        if( maybe_tile.has_value() ) {
          auto actions = click_on_world_tile( *maybe_tile );
          if( !actions.add_to_back.empty() ) {
            GET_CHECK_VARIANT( blink_unit, g_viewport_state,
                               viewport_state::blink_unit );
            // Note: the blink_unit.add_to_back vector is not
            // necessarily empty at this point, because ids can
            // accumulate in that vector until something causes
            // the frame loop to exit.
            for( auto id : actions.add_to_back )
              blink_unit.add_to_back.push_back( id );
          }
          if( !actions.bring_to_front.empty() ) {
            GET_CHECK_VARIANT( blink_unit, g_viewport_state,
                               viewport_state::blink_unit );
            // This should be emptied each time it is filled.
            CHECK( blink_unit.prioritize.empty() );
            blink_unit.prioritize = actions.bring_to_front;
            // Filter out units that have already moved this turn
            // from being prioritized. This check is not strictly
            // necessary since the unit would be skipped anyway
            // if it has already moved this turn. But we do it
            // anyway because it makes things a bit easier for
            // the code that will be handling this prioritizing.
            util::remove_if(
                blink_unit.prioritize,
                L( unit_from_id( _ ).moved_this_turn() ) );

            auto orig_size = actions.bring_to_front.size();
            auto curr_size = blink_unit.prioritize.size();
            CHECK( curr_size <= orig_size );
            if( curr_size == 0 )
              ui::message_box(
                  "The selected unit(s) have already moved this "
                  "turn." );
            else if( curr_size < orig_size )
              ui::message_box(
                  "Some of the selected units have already "
                  "moved this turn." );
          }
          handled = true;
        }
      }
      switch_non_exhaustive;
    }
    return handled;
  }
  Plane::DragInfo can_drag( input::e_mouse_button button,
                            Coord origin ) override {
    if( button == input::e_mouse_button::r &&
        viewport().screen_coord_in_viewport( origin ) )
      return Plane::e_accept_drag::yes;
    return Plane::e_accept_drag::no;
  }
  void on_drag( input::mod_keys const& /*unused*/,
                input::e_mouse_button /*unused*/,
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

  static auto rect() {
    return compositor::section( compositor::e_section::panel );
  }
  static W panel_width() { return rect().w; }
  static H panel_height() { return rect().h; }

  Delta delta() const { return {panel_width(), panel_height()}; }
  Coord origin() const { return rect().upper_left(); };

  ui::ButtonView& next_turn_button() {
    auto p_view = view->at( 0 );
    return *p_view.view->cast<ui::ButtonView>();
  }

  void initialize() override {
    vector<ui::OwningPositionedView> view_vec;

    auto button_view =
        make_unique<ui::ButtonView>( "Next Turn", [this] {
          lg.debug( "on to next turn." );
          this->next_turn_clicked = true;
          // Disable the button as soon as it is clicked.
          this->next_turn_button().enable( /*enabled=*/false );
        } );
    button_view->blink( /*enabled=*/true );

    auto button_size = button_view->delta();
    auto where =
        Coord{} + ( panel_width() / 2 ) - ( button_size.w / 2 );
    where += 16_h;

    ui::OwningPositionedView p_view( std::move( button_view ),
                                     where );
    view_vec.emplace_back( std::move( p_view ) );

    view = make_unique<ui::InvisibleView>(
        delta(), std::move( view_vec ) );

    next_turn_button().enable( false );
  }

  void draw( Texture& tx ) const override {
    clear_texture_transparent( tx );
    auto left_side =
        0_x + main_window_logical_size().w - panel_width();

    auto const& wood = lookup_sprite( g_tile::wood_middle );
    auto        wood_width  = wood.size().w;
    auto        wood_height = wood.size().h;

    for( Y i = rect().top_edge(); i < rect().bottom_edge();
         i += wood_height )
      render_sprite( tx, g_tile::wood_middle, i,
                     left_side + wood_width, 0, 0 );
    for( Y i = rect().top_edge(); i < rect().bottom_edge();
         i += wood_height )
      render_sprite( tx, g_tile::wood_left_edge, i, left_side, 0,
                     0 );

    view->draw( tx, origin() );
  }

  bool input( input::event_t const& event ) override {
    // FIXME: we need a window manager in the panel to avoid du-
    // plicating logic between here and the window module.
    if( input::is_mouse_event( event ) ) {
      auto maybe_pos = input::mouse_position( event );
      CHECK( maybe_pos.has_value() );
      if( maybe_pos.value().get().is_inside( rect() ) ) {
        auto new_event =
            move_mouse_origin_by( event, origin() - Coord{} );
        (void)view->input( new_event );
        return true;
      }
      return false;
    } else
      return view->input( event );
  }

  void mark_end_of_turn() {
    next_turn_button().enable( /*enabled=*/true );
    next_turn_clicked = false;
  }
  bool was_next_turn_button_clicked() const {
    return next_turn_clicked;
  }

  UPtr<ui::InvisibleView> view;
  bool                    next_turn_clicked{false};
};

PanelPlane g_panel_plane;

} // namespace

Plane* panel_plane() { return &g_panel_plane; }

void mark_end_of_turn() { g_panel_plane.mark_end_of_turn(); }

bool was_next_turn_button_clicked() {
  return g_panel_plane.was_next_turn_button_clicked();
}

namespace {

struct EffectsPlane : public Plane {
  EffectsPlane() = default;
  bool enabled() const override { return enabled_; }
  bool covers_screen() const override { return false; }
  void draw( Texture& tx ) const override {
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
                      main_window_logical_rect() );
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
