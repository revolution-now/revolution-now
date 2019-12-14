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
#include "no-serial.hpp"
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

Texture       g_tx_depixelate_from;
Vec<Coord>    g_pixels;
Matrix<Color> g_demoted_pixels;

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
    LandViewState,                              //
    ( none ),                                   //
    ( future,                                   //
      ( no_serial<sync_future<>>, s_future ) ), //
    ( blinking_unit,                            //
      ( UnitId, id ),                           //
      // Units that the player has asked to add to the orders
      // queue but at the end. This is useful if a unit that is
      // sentry'd has already been removed from the queue
      // (without asking for orders) and later in the same turn
      // had its orders cleared by the player (but not priori-
      // tized), this will allow it to ask for orders this turn.
      ( Vec<UnitId>, add_to_back ) ),               //
    ( input_ready,                                  //
      ( bool, consumed ),                           //
      ( no_serial<UnitInputResponse>, response ) ), //
    ( sliding_unit,                                 //
      ( UnitId, id ),                               //
      ( Coord, target ),                            //
      ( double, percent ),                          //
      ( DissipativeVelocity, percent_vel ) ),       //
    ( depixelating_unit,                            //
      ( UnitId, id ),                               //
      ( Opt<e_unit_type>, demoted ) )               //
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
  ((input_ready,       end),  ->,  none                ),
  ((input_ready,   input_orders    ),  ->,  input_ready  ),
  ((blinking_unit, input_orders    ),  ->,  input_ready  ),
  ((blinking_unit, input_prioritize),  ->,  input_ready  ),
  ((blinking_unit, add_to_back     ),  ->,  blinking_unit),
);
// clang-format on

fsm_class( LandView ) { //
  fsm_init( LandViewState::none{} );

  fsm_transition( LandView, blinking_unit, input_orders, ->,
                  input_ready ) {
    return { /*consumed=*/false, //
             UnitInputResponse{
                 /*id=*/cur.id,                  //
                 /*orders=*/event.orders,        //
                 /*add_to_front=*/{},            //
                 /*add_to_back=*/cur.add_to_back //
             } };
  }

  fsm_transition( LandView, input_ready, input_orders, ->,
                  input_ready ) {
    return { /*consumed=*/false, //
             UnitInputResponse{
                 /*id=*/cur.response->id,                     //
                 /*orders=*/event.orders,                     //
                 /*add_to_front=*/cur.response->add_to_front, //
                 /*add_to_back=*/cur.response->add_to_back    //
             } };
  }

  fsm_transition( LandView, blinking_unit, input_prioritize, ->,
                  input_ready ) {
    return { /*consumed=*/false, //
             UnitInputResponse{
                 /*id=*/cur.id,                     //
                 /*orders=*/nullopt,                //
                 /*add_to_front=*/event.prioritize, //
                 /*add_to_back=*/cur.add_to_back    //
             } };
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
    auto coord = coord_for_unit_indirect( event.id );
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
        /*id=*/event.id,           //
        /*demoted=*/maybe_demoted, //
    };
    g_pixels.clear();
    g_demoted_pixels.clear();
    g_pixels.assign( g_tile_rect.begin(), g_tile_rect.end() );
    rng::shuffle( g_pixels );
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
      g_demoted_pixels = tx.pixels();
    }
    return res;
  }
};

FSM_DEFINE_FORMAT_RN_( LandView );

/****************************************************************
** Animation State
*****************************************************************/
// Holds which animation we are currently in. A given animation
// may involve multiple lower-level steps, each of which might be
// represented by a different LandViewState.
adt_rn_( LandViewAnim,
         ( none ),                          //
         ( move,                            //
           ( UnitId, id ),                  //
           ( e_direction, d ) ),            //
         ( attack,                          //
           ( UnitId, attacker ),            //
           ( UnitId, defender ),            //
           ( bool, attacker_wins ),         //
           ( e_depixelate_anim, dp_anim ) ) //
);

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
  LandViewAnim_t anim;
  // Non-serialized fields.

private:
  SAVEGAME_FRIENDS( LandView );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    // This won't recreate the previous contents of the texture,
    // but it will be enough, since it only matters if the game
    // happens to be saved while a unit is being depixelated.
    g_tx_depixelate_from = create_texture( g_tile_delta );
    clear_texture_transparent( g_tx_depixelate_from );

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

void advance_viewport_state() {
  ASSIGN_CHECK_OPT(
      viewport_rect_pixels,
      compositor::section( compositor::e_section::viewport ) );

  SG().viewport.advance_state( viewport_rect_pixels,
                               world_size_tiles() );

  // TODO: should only do the following when the viewport has
  // input focus.
  auto const* __state = ::SDL_GetKeyboardState( nullptr );

  // Returns true if key is pressed.
  auto state = [__state]( ::SDL_Scancode code ) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return __state[code] != 0;
  };

  if( state( ::SDL_SCANCODE_LSHIFT ) ) {
    SG().viewport.set_x_push(
        state( ::SDL_SCANCODE_A )
            ? e_push_direction::negative
            : state( ::SDL_SCANCODE_D )
                  ? e_push_direction::positive
                  : e_push_direction::none );
    // y motion
    SG().viewport.set_y_push(
        state( ::SDL_SCANCODE_W )
            ? e_push_direction::negative
            : state( ::SDL_SCANCODE_S )
                  ? e_push_direction::positive
                  : e_push_direction::none );

    if( state( ::SDL_SCANCODE_A ) || state( ::SDL_SCANCODE_D ) ||
        state( ::SDL_SCANCODE_W ) || state( ::SDL_SCANCODE_S ) )
      SG().viewport.stop_auto_panning();
  }
}

void advance_landview_anim_state() {
  if( util::holds<LandViewAnim::none>( SG().anim ) ) {
    // We're not supposed to be animating anything.
    switch_( SG().mode.state() ) {
      case_( LandViewState::none ) {}
      case_( LandViewState::future ) {}
      case_( LandViewState::blinking_unit ) {}
      case_( LandViewState::input_ready ) {}
      case_( LandViewState::sliding_unit ) {
        SHOULD_NOT_BE_HERE;
      }
      case_( LandViewState::depixelating_unit ) {
        SHOULD_NOT_BE_HERE;
      }
      switch_exhaustive;
    }
    return;
  }
  // We're supposed to be animating something.
  if( SG().mode.holds<LandViewState::none>() ) {
    // Kick off the animation.
    switch_( SG().anim ) {
      case_( LandViewAnim::none ) { SHOULD_NOT_BE_HERE; }
      case_( LandViewAnim::move ) {
        SG().mode.send_event( LandViewEvent::slide_unit{
            /*id=*/val.id,      //
            /*direction=*/val.d //
        } );
      }
      case_( LandViewAnim::attack ) {
        ASSIGN_CHECK_OPT( attacker_coord,
                          coord_for_unit( val.attacker ) );
        ASSIGN_CHECK_OPT( defender_coord,
                          coord_for_unit( val.defender ) );
        ASSIGN_CHECK_OPT(
            d, attacker_coord.direction_to( defender_coord ) );
        SG().mode.send_event( LandViewEvent::slide_unit{
            /*id=*/val.attacker, //
            /*direction=*/d      //
        } );
      }
      switch_exhaustive;
    }
    return;
  }
  // We should already be animating something.
  switch_( SG().mode.state() ) {
    case_( LandViewState::none ) { SHOULD_NOT_BE_HERE; }
    case_( LandViewState::future ) { SHOULD_NOT_BE_HERE; }
    case_( LandViewState::blinking_unit ) { SHOULD_NOT_BE_HERE; }
    case_( LandViewState::input_ready ) { SHOULD_NOT_BE_HERE; }
    case_( LandViewState::sliding_unit ) {}
    case_( LandViewState::depixelating_unit ) {}
    switch_exhaustive;
  }
  bool finished_anim = false;
  switch_( SG().anim ) {
    case_( LandViewAnim::none ) { SHOULD_NOT_BE_HERE; }
    case_( LandViewAnim::move ) {
      ASSIGN_CHECK_OPT(
          sliding,
          SG().mode.holds<LandViewState::sliding_unit>() );
      // Are we finished?
      if( sliding.get().percent >= 1.0 ) {
        finished_anim = true;
        SG().mode.send_event( LandViewEvent::end{} );
      }
    }
    case_( LandViewAnim::attack ) {
      CHECK( util::holds<LandViewState::sliding_unit>(
                 SG().mode.state() ) ||
             util::holds<LandViewState::depixelating_unit>(
                 SG().mode.state() ) );
      if_v( SG().mode.state(), LandViewState::sliding_unit,
            sliding ) {
        if( sliding->percent >= 1.0 ) {
          SG().mode.send_event( LandViewEvent::end{} );
          if( val.dp_anim == e_depixelate_anim::none ) {
            finished_anim = true;
          } else {
            // Move on to depixelation.
            bool demote =
                ( val.dp_anim == e_depixelate_anim::demote );
            SG().mode.send_event( LandViewEvent::depixelate_unit{
                /*id=*/val.attacker_wins ? val.defender
                                         : val.attacker, //
                /*demote=*/demote                        //
            } );
          }
        }
      }
      if_v( SG().mode.state(), LandViewState::depixelating_unit,
            val ) {
        if( g_pixels.empty() ) {
          SG().mode.send_event( LandViewEvent::end{} );
          finished_anim = true;
        }
      }
    }
    switch_exhaustive;
  }
  if( finished_anim ) SG().anim = LandViewAnim::none{};
}

// Will be called repeatedly until no more events added to fsm.
void advance_landview_state( LandViewFsm& fsm ) {
  switch_( fsm.mutable_state() ) {
    case_( LandViewState::none ) {}
    case_( LandViewState::future, s_future ) {
      advance_fsm_ui_state( &fsm, &s_future.o );
    }
    case_( LandViewState::blinking_unit ) {
      // FIXME: add blinking state here.
    }
    case_( LandViewState::input_ready ) {}
    case_( LandViewState::sliding_unit ) {
      ASSIGN_CHECK_OPT(
          slide, fsm.holds<LandViewState::sliding_unit>() );
      slide.get().percent_vel.advance( e_push_direction::none );
      slide.get().percent += slide.get().percent_vel.to_double();
      if( slide.get().percent > 1.0 ) slide.get().percent = 1.0;
    }
    case_( LandViewState::depixelating_unit ) {
      if( !g_pixels.empty() ) {
        int to_depixelate =
            std::min( config_rn.depixelate_pixels_per_frame,
                      int( g_pixels.size() ) );
        vector<Coord> new_non_pixels;
        for( int i = 0; i < to_depixelate; ++i ) {
          auto next_coord = g_pixels.back();
          g_pixels.pop_back();
          new_non_pixels.push_back( next_coord );
        }
        ::SDL_SetRenderDrawBlendMode( g_renderer,
                                      ::SDL_BLENDMODE_NONE );
        for( auto point : new_non_pixels ) {
          auto color = g_demoted_pixels.size().area() > 0
                           ? g_demoted_pixels[point]
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
** Tile Clicking
*****************************************************************/
// If this is default constructed then it should represent "no
// actions need to be taken."
struct ClickTileActions {
  Vec<UnitId> bring_to_front{};
  Vec<UnitId> add_to_back{};
};
NOTHROW_MOVE( ClickTileActions );

void ProcessClickTileActions( ClickTileActions const& actions ) {
  if( !actions.add_to_back.empty() ) {
    SG().mode.send_event( LandViewEvent::add_to_back{
        /*ids=*/actions.add_to_back } );
  }
  if( !actions.bring_to_front.empty() ) {
    auto prioritize = actions.bring_to_front;
    util::remove_if( prioritize,
                     L( unit_from_id( _ ).mv_pts_exhausted() ) );
    auto orig_size = actions.bring_to_front.size();
    auto curr_size = prioritize.size();
    CHECK( curr_size <= orig_size );
    if( curr_size == 0 ) {
      SG().mode.push( LandViewState::future{
          ui::message_box( "The selected unit(s) have already "
                           "moved this turn." ) } );
    } else {
      SG().mode.send_event( LandViewEvent::input_prioritize{
          /*prioritize=*/prioritize } );
      if( curr_size < orig_size ) {
        SG().mode.push( LandViewState::future{
            ui::message_box( "Some of the selected units have "
                             "already moved this turn." ) } );
      }
    }
  }
}

ClickTileActions ClickTileActionsFromUnitSelections(
    Vec<ui::UnitSelection> const& selections,
    bool                          allow_activate ) {
  ClickTileActions result{};
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

// If there is a single unit on the square with orders and
// allow_activate is false then the unit's orders will be
// cleared.
//
// If there is a single unit on the square with orders and
// allow_activate is true then the unit's orders will be cleared
// and the unit will be placed at the back of the queue to poten-
// tially move this turn.
//
// If there is a single unit on the square with no orders and
// allow_activate is false then nothing is done.
//
// If there is a single unit on the square with no orders and
// allow_activate is true then the unit will be prioritized
// (moved to the front of the queue).
//
// If there are multiple units on the square then it will pop
// open a window to allow the user to select and/or activate
// them, with the results for each unit behaving in a similar way
// to the single-unit case described above with respect to orders
// and the allow_activate flag.
sync_future<ClickTileActions> click_on_world_tile_impl(
    Coord coord, bool allow_activate ) {
  auto const& units = units_from_coord_recursive( coord );
  if( units.size() == 0 )
    return make_sync_future<ClickTileActions>();

  sync_future<Vec<ui::UnitSelection>> s_future;
  if( units.size() == 1 ) {
    auto              id = *units.begin();
    ui::UnitSelection selection{
        id, ui::e_unit_selection::clear_orders };
    if( !unit_from_id( id ).has_orders() && allow_activate )
      selection.what = ui::e_unit_selection::activate;
    s_future = make_sync_future<Vec<ui::UnitSelection>>(
        vector{ selection } );
  } else {
    s_future = ui::unit_selection_box( units, allow_activate );
  }

  return s_future.then(
      [allow_activate](
          Vec<ui::UnitSelection> const& selections ) {
        return ClickTileActionsFromUnitSelections(
            selections, allow_activate );
      } );
}

// This function will handle all the actions that can happen as a
// result of the player "clicking" on a world tile. This can in-
// clude activiting units, popping up windows, etc.
sync_future<ClickTileActions> click_on_world_tile(
    Coord coord ) {
  auto s_future = make_sync_future<ClickTileActions>();
  if( !util::holds<LandViewAnim::none>( SG().anim ) )
    return s_future;
  switch_( SG().mode.state() ) {
    case_( LandViewState::none ) {
      s_future = click_on_world_tile_impl(
          coord, /*allow_activate=*/false );
    }
    case_( LandViewState::blinking_unit ) {
      s_future =
          click_on_world_tile_impl( coord,
                                    /*allow_activate=*/true );
    }
    switch_non_exhaustive;
  }
  return s_future;
}

struct LandViewPlane : public Plane {
  LandViewPlane() = default;
  bool covers_screen() const override { return true; }
  void advance_state() override {
    // Order matters here.
    fsm_auto_advance( SG().mode, "land-view",
                      { advance_landview_state } );
    advance_landview_anim_state();
    advance_viewport_state();
  }
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
  e_input_handled input( input::event_t const& event ) override {
    bool hold = matcher_( SG().mode.state() ) {
      case_( LandViewState::none ) { return false; }
      case_( LandViewState::future ) { return false; }
      case_( LandViewState::sliding_unit ) { return true; }
      case_( LandViewState::depixelating_unit ) { return true; }
      case_( LandViewState::input_ready ) { return true; }
      case_( LandViewState::blinking_unit ) { return false; }
      matcher_exhaustive;
    };
    if( hold ) return e_input_handled::hold;
    auto handled = e_input_handled::no;
    switch_( event ) {
      case_( input::unknown_event_t ) {}
      case_( input::quit_event_t ) {}
      case_( input::win_event_t ) {}
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
          case_( LandViewState::future ) {}
          case_( LandViewState::sliding_unit ) {}
          case_( LandViewState::depixelating_unit ) {}
          case_( LandViewState::input_ready ) {
            // Swallow further inputs.
            handled = e_input_handled::yes;
          }
          case_( LandViewState::blinking_unit ) {
            auto& blink_unit = val;
            handled          = e_input_handled::yes;
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
                handled = e_input_handled::no;
                if( key_event.direction ) {
                  SG().mode.send_event(
                      LandViewEvent::input_orders{
                          /*orders=*/orders::direction{
                              *key_event.direction } } );
                  handled = e_input_handled::yes;
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
          handled = e_input_handled::yes;
        }
      }
      case_( input::mouse_button_event_t ) {
        if( val.buttons != input::e_mouse_button_event::left_up )
          break_;
        if( auto maybe_tile =
                SG().viewport.screen_pixel_to_world_tile(
                    val.pos ) ) {
          SG().mode.push( LandViewState::future{
              click_on_world_tile( *maybe_tile )
                  .consume( ProcessClickTileActions ) } );
          handled = e_input_handled::yes;
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
Opt<UnitInputResponse> unit_input_response() {
  Opt<UnitInputResponse> res;
  if_v( SG().mode.mutable_state(), LandViewState::input_ready,
        val ) {
    if( !val->consumed ) {
      SG().mode.send_event( LandViewEvent::end{} );
      res           = std::move( val->response.o );
      val->consumed = true;
    }
  }
  return res;
}

void landview_do_eot() {
  // nothing?
}

void landview_ensure_unit_visible( UnitId id ) {
  auto coords = coord_for_unit_indirect( id );
  SG().viewport.ensure_tile_visible( coords,
                                     /*smooth=*/true );
}

void landview_ask_orders( UnitId id ) {
  landview_ensure_unit_visible( id );
  SG().mode.send_event( LandViewEvent::blink_unit{ /*id=*/id } );
}

bool landview_is_animating() {
  return !util::holds<LandViewAnim::none>( SG().anim );
}

void landview_animate_move( UnitId id, e_direction direction ) {
  landview_ensure_unit_visible( id );
  CHECK( util::holds<LandViewAnim::none>( SG().anim ) );
  SG().anim = LandViewAnim::move{
      /*id=*/id,      //
      /*d=*/direction //
  };
}

void landview_animate_attack( UnitId attacker, UnitId defender,
                              bool              attacker_wins,
                              e_depixelate_anim dp_anim ) {
  landview_ensure_unit_visible( attacker );
  CHECK( util::holds<LandViewAnim::none>( SG().anim ) );
  SG().anim = LandViewAnim::attack{
      /*attacker=*/attacker,           //
      /*defender=*/defender,           //
      /*attacker_wins=*/attacker_wins, //
      /*dp_anim=*/dp_anim              //
  };
}

/****************************************************************
** Testing
*****************************************************************/
void test_land_view() {
  //
}

} // namespace rn
