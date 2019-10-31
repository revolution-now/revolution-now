/****************************************************************
**land-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-29.
*
* Description: Handles the main game view of the land.
*
*****************************************************************/
#include "land-view.hpp"

// Revolution Now
#include "adt.hpp"
#include "aliases.hpp"
#include "compositor.hpp"
#include "config-files.hpp"
#include "coord.hpp"
#include "fb.hpp"
#include "fsm.hpp"
#include "gfx.hpp"
#include "id.hpp"
#include "logging.hpp"
#include "matrix.hpp"
#include "orders.hpp"
#include "physics.hpp"
#include "plane.hpp"
#include "rand.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "tx.hpp"
#include "ustate.hpp"
#include "utype.hpp"
#include "viewport.hpp"
#include "window.hpp"

// Revolution Now (config)
#include "../config/ucl/rn.inl"

// Flatbuffers
#include "fb/sg-land-view_generated.h"

using namespace std;

namespace rn {

namespace {

Texture            g_tx_depixelate_from;
Opt<Matrix<Color>> g_demote_pixels{};

/****************************************************************
** FSMs
*****************************************************************/
// The land-view rendering states are not really states of the
// world, they are mainly just animation or rendering states.
// Each state is represented by a struct which may contain data
// members. The data members of the struct's will be mutated in
// in order to change/advance the state of animation, although
// the rendering functions themselves will never mutate them.
adt_s_rn_(
    LandViewState,    //
    ( none ),         //
    ( blinking_unit,  //
      ( UnitId, id ), //
      // Units that the player has asked to add to the orders
      // queue but at the end. This is useful if a unit that is
      // sentry'd has already been removed from the queue
      // (without asking for orders) and later in the same turn
      // had its orders cleared by the player (but not priori-
      // tized), this will allow it to ask for orders this turn.
      ( Vec<UnitId>, add_to_back ) ), //
    ( input_ready,                    //
      ( UnitId, id ),                 //
      ( Opt<orders_t>, orders ),      //
      // Units that the player has asked to prioritize (i.e.,
      // bring them forward in the queue of units waiting for or-
      // ders).
      ( Vec<UnitId>, prioritize ),            //
      ( Vec<UnitId>, add_to_back ) ),         //
    ( sliding_unit,                           //
      ( UnitId, id ),                         //
      ( Coord, target ),                      //
      ( double, percent ),                    //
      ( DissipativeVelocity, percent_vel ) ), //
    ( depixelating_unit,                      //
      ( UnitId, id ),                         //
      ( Vec<Coord>, pixels ),                 //
      ( Opt<e_unit_type>, demoted ) )         //
);

adt_rn_( LandViewEvent,                   //
         ( end ),                         //
         ( blink_unit,                    //
           ( UnitId, id ) ),              //
         ( add_to_back,                   //
           ( Vec<UnitId>, ids ) ),        //
         ( input_orders,                  //
           ( orders_t, orders ) ),        //
         ( input_prioritize,              //
           ( Vec<UnitId>, prioritize ) ), //
         ( slide_unit,                    //
           ( UnitId, id ),                //
           ( e_direction, direction ) ),  //
         ( depixelate_unit,               //
           ( UnitId, id ),                //
           ( bool, demote ) )             //
);

// clang-format off
fsm_transitions( LandView,
  ((none, blink_unit      ),  ->,  blinking_unit       ),
  ((none, slide_unit      ),  ->,  sliding_unit        ),
  ((none, depixelate_unit ),  ->,  depixelating_unit   ),
  ((sliding_unit,      end),  ->,  none                ),
  ((depixelating_unit, end),  ->,  none                ),
  ((blinking_unit, input_orders    ),  ->,  input_ready  ),
  ((blinking_unit, input_prioritize),  ->,  input_ready  ),
  ((blinking_unit, add_to_back     ),  ->,  blinking_unit),
);
// clang-format on

fsm_class( LandView ) { //
  fsm_init( LandViewState::none{} );

  fsm_transition( LandView, blinking_unit, input_orders, ->,
                  input_ready ) {
    return {
        /*id=*/cur.id,                  //
        /*orders=*/event.orders,        //
        /*prioritize=*/{},              //
        /*add_to_back=*/cur.add_to_back //
    };
  }

  fsm_transition( LandView, blinking_unit, input_prioritize, ->,
                  input_ready ) {
    return {
        /*id=*/cur.id,                   //
        /*orders=*/nullopt,              //
        /*prioritize=*/event.prioritize, //
        /*add_to_back=*/cur.add_to_back  //
    };
  }

  fsm_transition( LandView, blinking_unit, add_to_back, ->,
                  blinking_unit ) {
    Vec<UnitId> new_ids;
    new_ids.reserve( cur.add_to_back.size() + event.ids.size() );
    for( auto id : cur.add_to_back ) new_ids.push_back( id );
    for( auto id : event.ids ) new_ids.push_back( id );
    return {
        /*id=*/cur.id,                       //
        /*add_to_back=*/std::move( new_ids ) //
    };
  }

  fsm_transition( LandView, none, blink_unit, ->,
                  blinking_unit ) {
    (void)cur;
    return {
        /*id=*/event.id,   //
        /*add_to_back=*/{} //
    };
  }

  fsm_transition( LandView, none, slide_unit, ->,
                  sliding_unit ) {
    (void)cur;
    ASSIGN_CHECK_OPT( coord, coord_for_unit( event.id ) );
    // FIXME: check if target is in world.
    auto target = coord.moved( event.direction );
    return {
        /*id=*/event.id,   //
        /*target=*/target, //
        /*percent=*/0.0,   //
        /*percent_vel=*/
        DissipativeVelocity{
            /*min_velocity=*/0,            //
            /*max_velocity=*/.07,          //
            /*initial_velocity=*/.1,       //
            /*mag_acceleration=*/1,        //
            /*mag_drag_acceleration=*/.002 //
        }                                  //
    };
  }

  fsm_transition( LandView, none, depixelate_unit, ->,
                  depixelating_unit ) {
    (void)cur;
    Opt<e_unit_type> maybe_demoted;
    if( event.demote ) {
      maybe_demoted = unit_from_id( event.id ).desc().demoted;
      CHECK( maybe_demoted.has_value(),
             "cannot demote {} because it is not demotable.",
             debug_string( event.id ) );
    }
    auto res = LandViewState::depixelating_unit{
        /*id=*/event.id,          //
        /*pixels=*/{},            //
        /*demoted=*/maybe_demoted //
    };
    res.pixels.assign( g_tile_rect.begin(), g_tile_rect.end() );
    rng::shuffle( res.pixels );
    g_tx_depixelate_from = create_texture( g_tile_delta );
    clear_texture_transparent( g_tx_depixelate_from );
    render_unit( g_tx_depixelate_from, res.id, Coord{},
                 /*with_icon=*/true );
    // Now, if we are depixelating to another unit then we will
    // set that process up.
    if( maybe_demoted.has_value() ) {
      auto tx = Texture::create( g_tile_delta );
      clear_texture_transparent( tx );
      auto& unit = unit_from_id( res.id );
      // Render the target unit with no nationality icon, then
      // render the nationality icon of the unit being depixe-
      // lated so that it doesn't appear to change during the an-
      // imation.
      if( !unit.desc().nat_icon_front ) {
        render_unit( tx, *maybe_demoted, Coord{} );
        render_nationality_icon( tx, res.id, Coord{} );
      } else {
        render_nationality_icon( tx, res.id, Coord{} );
        render_unit( tx, *maybe_demoted, Coord{} );
      }
      g_demote_pixels = tx.pixels();
    }
    return res;
  }
};

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( LandView ) {
  // Fields that are actually serialized.

  // clang-format off
  SAVEGAME_MEMBERS( LandView,
  ( LandViewFsm,    mode ),
  ( SmoothViewport, viewport ));
  // clang-format on

public:
  // Non-serialized fields.

private:
  SAVEGAME_FRIENDS( LandView );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    g_tx_depixelate_from = create_texture( g_tile_delta );

    ASSIGN_CHECK_OPT(
        viewport_rect_pixels,
        compositor::section( compositor::e_section::viewport ) );
    // This call is needed after construction to initialize the
    // invariants. It is expected that the parameters of this
    // function will not depend on any other deserialized data,
    // but only on data available after the init routines run.
    viewport.advance_state( viewport_rect_pixels,
                            world_size_tiles() );
    return xp_success_t{};
  }
};
SAVEGAME_IMPL( LandView );

/****************************************************************
** Land-View Rendering
*****************************************************************/
void render_land_view() {
  auto const& state = SG().mode.state();
  g_texture_viewport.set_render_target();

  auto covered = SG().viewport.covered_tiles();

  render_terrain( covered, g_texture_viewport, Coord{} );

  Opt<Coord>  blink_coords;
  Opt<UnitId> blink_id;
  // if( util::holds( state, LandViewState::blinking_unit
  if_v( state, LandViewState::blinking_unit, blink ) {
    blink_coords = coord_for_unit_indirect( blink->id );
    blink_id     = blink->id;
  }

  Opt<UnitId> slide_id;
  if_v( state, LandViewState::sliding_unit, slide ) {
    slide_id = slide->id;
  }

  Opt<UnitId> depixelate_id;
  if_v( state, LandViewState::depixelating_unit, dying ) {
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
  if_v( state, LandViewState::sliding_unit, slide ) {
    Coord coords = coord_for_unit_indirect( slide->id );
    Delta delta  = slide->target - coords;
    CHECK( -1 <= delta.w && delta.w <= 1 );
    CHECK( -1 <= delta.h && delta.h <= 1 );
    delta *= g_tile_scale;
    Delta pixel_delta{ W( int( delta.w._ * slide->percent ) ),
                       H( int( delta.h._ * slide->percent ) ) };

    auto  covered = SG().viewport.covered_tiles();
    Coord pixel_coord =
        Coord{} + ( coords - covered.upper_left() );
    pixel_coord *= g_tile_scale;
    pixel_coord += pixel_delta;
    render_unit( g_texture_viewport, slide->id, pixel_coord,
                 /*with_icon=*/true );
  }

  // Now do depixelation, if any.
  if_v( state, LandViewState::depixelating_unit, dying ) {
    ::SDL_SetRenderDrawBlendMode( g_renderer,
                                  ::SDL_BLENDMODE_BLEND );
    auto  covered = SG().viewport.covered_tiles();
    Coord coords  = coord_for_unit_indirect( dying->id );
    Coord pixel_coord =
        Coord{} + ( coords - covered.upper_left() );
    pixel_coord *= g_tile_scale;
    copy_texture( g_tx_depixelate_from, g_texture_viewport,
                  pixel_coord );
  }
}

// If this is default constructed then it should represent "no
// actions need to be taken."
struct ClickTileActions {
  Vec<UnitId> bring_to_front{};
  Vec<UnitId> add_to_back{};
};
NOTHROW_MOVE( ClickTileActions );

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
    TODO( "need to use async windows here." );
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

// This function will handle all the actions that can happen as
// a result of the player "clicking" on a world tile. This can
// in- clude activiting units, popping up windows, etc.
ClickTileActions click_on_world_tile( Coord coord ) {
  using Res_t = ClickTileActions;
  return matcher_( SG().mode.state(), ->, Res_t ) {
    case_( LandViewState::none ) {
      return click_on_world_tile_impl(
          coord, /*allow_activate=*/false );
    }
    case_( LandViewState::blinking_unit ) {
      return click_on_world_tile_impl( coord,
                                       /*allow_activate=*/true );
    }
    case_( LandViewState::input_ready ) {
      return Res_t{}; //
    }
    case_( LandViewState::sliding_unit ) {
      return Res_t{}; //
    }
    case_( LandViewState::depixelating_unit ) {
      return Res_t{}; //
    }
    matcher_exhaustive;
  }
}

struct LandViewPlane : public Plane {
  LandViewPlane() = default;
  bool covers_screen() const override { return true; }
  void draw( Texture& tx ) const override {
    render_land_view();
    clear_texture_black( tx );
    copy_texture_stretch( g_texture_viewport, tx,
                          SG().viewport.rendering_src_rect(),
                          SG().viewport.rendering_dest_rect() );
  }
  Opt<Plane::MenuClickHandler> menu_click_handler(
      e_menu_item item ) const override {
    // These are factors by which the zoom will be scaled when
    // zooming in/out with the menus.
    double constexpr zoom_in_factor  = 2.0;
    double constexpr zoom_out_factor = 1.0 / zoom_in_factor;
    // This is so that a zoom-in followed by a zoom-out will
    // re- store to previous state.
    static_assert( zoom_in_factor * zoom_out_factor == 1.0 );
    if( item == e_menu_item::zoom_in ) {
      static Plane::MenuClickHandler handler = [] {
        // A user zoom request halts any auto zooming that may
        // currently be happening.
        SG().viewport.stop_auto_zoom();
        SG().viewport.stop_auto_panning();
        SG().viewport.smooth_zoom_target(
            SG().viewport.get_zoom() * zoom_in_factor );
      };
      return handler;
    }
    if( item == e_menu_item::zoom_out ) {
      static Plane::MenuClickHandler handler = [] {
        // A user zoom request halts any auto zooming that may
        // currently be happening.
        SG().viewport.stop_auto_zoom();
        SG().viewport.stop_auto_panning();
        SG().viewport.smooth_zoom_target(
            SG().viewport.get_zoom() * zoom_out_factor );
      };
      return handler;
    }
    if( item == e_menu_item::restore_zoom ) {
      if( SG().viewport.get_zoom() == 1.0 ) return nullopt;
      static Plane::MenuClickHandler handler = [] {
        SG().viewport.smooth_zoom_target( 1.0 );
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
        // This is because we need to distinguish uppercase
        // from lowercase.
        if( state( ::SDL_SCANCODE_LSHIFT ) ||
            state( ::SDL_SCANCODE_RSHIFT ) )
          break_;

        auto& key_event = val;
        if( key_event.change != input::e_key_change::down )
          break_;
        switch_( SG().mode.state() ) {
          case_( LandViewState::none ) {
            switch( key_event.keycode ) {
              case ::SDLK_z:
                SG().viewport.smooth_zoom_target( 1.0 );
                break;
              default: break;
            }
          }
          case_( LandViewState::sliding_unit ) {}
          case_( LandViewState::depixelating_unit ) {}
          case_( LandViewState::input_ready ) {
            // Swallow further inputs.
            handled = true;
          }
          case_( LandViewState::blinking_unit ) {
            auto& blink_unit = val;
            handled          = true;
            switch( key_event.keycode ) {
              case ::SDLK_z:
                SG().viewport.smooth_zoom_target( 1.0 );
                break;
              case ::SDLK_w:
                SG().mode.send_event(
                    LandViewEvent::input_orders{
                        /*orders=*/orders::wait{} } );
                break;
              case ::SDLK_s:
                SG().mode.send_event(
                    LandViewEvent::input_orders{
                        /*orders=*/orders::sentry{} } );
                break;
              case ::SDLK_f:
                SG().mode.send_event(
                    LandViewEvent::input_orders{
                        /*orders=*/orders::fortify{} } );
                break;
              case ::SDLK_c:
                SG().viewport.ensure_tile_visible(
                    coord_for_unit_indirect( blink_unit.id ),
                    /*smooth=*/true );
                break;
              case ::SDLK_d:
                SG().mode.send_event(
                    LandViewEvent::input_orders{
                        /*orders=*/orders::disband{} } );
                break;
              case ::SDLK_SPACE:
              case ::SDLK_KP_5:
                SG().mode.send_event(
                    LandViewEvent::input_orders{
                        /*orders=*/orders::forfeight{} } );
                break;
              default:
                handled = false;
                if( key_event.direction ) {
                  SG().mode.send_event(
                      LandViewEvent::input_orders{
                          /*orders=*/orders::direction{
                              *key_event.direction } } );
                  handled = true;
                }
                break;
            }
          }
          switch_exhaustive;
        }
      }
      case_( input::mouse_wheel_event_t ) {
        // If the mouse is in the viewport and its a wheel
        // event then we are in business.
        if( SG().viewport.screen_coord_in_viewport( val.pos ) ) {
          if( val.wheel_delta < 0 )
            SG().viewport.set_zoom_push(
                e_push_direction::negative, nullopt );
          if( val.wheel_delta > 0 )
            SG().viewport.set_zoom_push(
                e_push_direction::positive, val.pos );
          // A user zoom request halts any auto zooming that
          // may currently be happening.
          SG().viewport.stop_auto_zoom();
          SG().viewport.stop_auto_panning();
          handled = true;
        }
      }
      case_( input::mouse_button_event_t ) {
        if( val.buttons != input::e_mouse_button_event::left_up )
          break_;
        auto maybe_tile =
            SG().viewport.screen_pixel_to_world_tile( val.pos );
        // See if the cursor has clicked on a tile in the
        // viewport.
        if( maybe_tile.has_value() ) {
          auto actions = click_on_world_tile( *maybe_tile );
          if( !actions.add_to_back.empty() ) {
            SG().mode.send_event( LandViewEvent::add_to_back{
                /*ids=*/actions.add_to_back } );
          }
          // !! Must handle the add_to_back first before priori-
          // tize, since prioritize will lead to a state change.
          if( !actions.bring_to_front.empty() ) {
            // Must copy (see below).
            auto prioritize = actions.bring_to_front;
            // Filter out units that have already moved this
            // turn from being prioritized. This check is not
            // strictly necessary since the unit would be
            // skipped anyway if it has already moved this
            // turn. But we do it anyway because it makes
            // things a bit easier for the code that will be
            // handling this prioritizing.
            util::remove_if(
                prioritize,
                L( unit_from_id( _ ).moved_this_turn() ) );

            auto orig_size = actions.bring_to_front.size();
            auto curr_size = prioritize.size();
            CHECK( curr_size <= orig_size );
            if( curr_size == 0 ) {
              ui::message_box(
                  "The selected unit(s) have already moved "
                  "this turn." );
            } else {
              if( curr_size < orig_size )
                ui::message_box(
                    "Some of the selected units have already "
                    "moved this turn." );
              SG().mode.send_event(
                  LandViewEvent::input_prioritize{
                      /*prioritize=*/prioritize } );
            }
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
        SG().viewport.screen_coord_in_viewport( origin ) )
      return Plane::e_accept_drag::yes;
    return Plane::e_accept_drag::no;
  }
  void on_drag( input::mod_keys const& /*unused*/,
                input::e_mouse_button /*unused*/,
                Coord /*unused*/, Coord prev,
                Coord current ) override {
    SG().viewport.stop_auto_panning();
    // When the mouse drags up, we need to move the viewport
    // center down.
    SG().viewport.pan_by_screen_coords( prev - current );
  }
};

LandViewPlane g_land_view_plane;

} // namespace

Plane* land_view_plane() { return &g_land_view_plane; }

/****************************************************************
** Public API
*****************************************************************/
void advance_landview_state() {
  ASSIGN_CHECK_OPT(
      viewport_rect_pixels,
      compositor::section( compositor::e_section::viewport ) );

  // Must be done as early as possible.
  SG().mode.process_events();
  SG().viewport.advance_state( viewport_rect_pixels,
                               world_size_tiles() );

  switch_( SG().mode.state() ) {
    case_( LandViewState::none ) {
      //
    }
    case_( LandViewState::blinking_unit ) {
      // FIXME: add blinking state here.
    }
    case_( LandViewState::input_ready ) {
      //
    }
    case_( LandViewState::sliding_unit ) {
      ASSIGN_CHECK_OPT(
          slide,
          SG().mode.holds<LandViewState::sliding_unit>() );
      slide.get().percent_vel.advance( e_push_direction::none );
      slide.get().percent += slide.get().percent_vel.to_double();
      if( slide.get().percent > 1.0 ) slide.get().percent = 1.0;
    }
    case_( LandViewState::depixelating_unit ) {
      ASSIGN_CHECK_OPT(
          dying_ref,
          SG().mode.holds<LandViewState::depixelating_unit>() );
      auto& dying = dying_ref.get();
      if( dying.pixels.empty() )
        SG().mode.send_event( LandViewEvent::end{} );
      else {
        int to_depixelate =
            std::min( config_rn.depixelate_pixels_per_frame,
                      int( dying.pixels.size() ) );
        vector<Coord> new_non_pixels;
        for( int i = 0; i < to_depixelate; ++i ) {
          auto next_coord = dying.pixels.back();
          dying.pixels.pop_back();
          new_non_pixels.push_back( next_coord );
        }
        ::SDL_SetRenderDrawBlendMode( g_renderer,
                                      ::SDL_BLENDMODE_NONE );
        for( auto point : new_non_pixels ) {
          auto color = g_demote_pixels.has_value()
                           ? ( *g_demote_pixels )[point]
                           : Color( 0, 0, 0, 0 );
          set_render_draw_color( color );
          g_tx_depixelate_from.set_render_target();
          ::SDL_RenderDrawPoint( g_renderer, point.x._,
                                 point.y._ );
        }
      }
    }
    switch_exhaustive;
  }
}

/****************************************************************
** Testing
*****************************************************************/
void test_land_view() {
  //
}

} // namespace rn
