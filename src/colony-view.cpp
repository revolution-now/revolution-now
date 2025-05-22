/****************************************************************
**colony-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-05.
*
* Description: The view that appears when clicking on a colony.
*
*****************************************************************/
#include "colony-view.hpp"

// Revolution Now
#include "cheat.hpp"
#include "co-combinator.hpp"
#include "colony-mgr.hpp"
#include "colony.hpp"
#include "colview-entities.hpp"
#include "drag-drop.hpp"
#include "icolony-evolve.rds.hpp"
#include "iengine.hpp"
#include "interrupts.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "screen.hpp" // FIXME: remove
#include "text.hpp"
#include "throttler.hpp"
#include "ts.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/units.hpp"

// render
#include "render/extra.hpp"
#include "render/renderer.hpp"

// refl
#include "refl/to-str.hpp"

// gfx
#include "gfx/pixel.hpp"

// base
#include "base/lambda.hpp"
#include "base/logger.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

struct IMapUpdater;

using ::gfx::e_resolution;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;

/****************************************************************
** Drawing
*****************************************************************/
void draw_colony_view( IEngine& engine, Colony const&,
                       rr::Renderer& renderer ) {
  static pixel const background_color =
      pixel::parse_from_hex( "f1cf81" ).value();
  draw_rect_noisy_filled(
      renderer,
      rect{ .origin = {},
            .size   = renderer.logical_screen_size() },
      background_color );
  auto const canvas = main_window_logical_rect(
      engine.video(), engine.window(), engine.resolutions() );
  colview_top_level().view().draw( renderer, canvas.nw() );
}

/****************************************************************
** Cheat Stuff
*****************************************************************/
void try_promote_demote_unit( SS& ss, TS& ts, Colony& colony,
                              Coord where, bool demote ) {
  maybe<DraggableObjectWithBounds<ColViewObject>> o =
      colview_top_level().object_here( where );
  if( !o.has_value() ) return;
  // Could be a commodity.
  maybe<UnitId> unit_id =
      o->obj.get_if<ColViewObject::unit>().member(
          &ColViewObject::unit::id );
  if( !unit_id.has_value() ) return;

  Unit& unit = ss.units.unit_for( *unit_id );
  if( demote )
    cheat_downgrade_unit_expertise( ss, ts, unit );
  else
    cheat_upgrade_unit_expertise( ss, ts, unit );
  update_colony_view( ss, colony );
}

void try_increase_commodity( SS& ss, Colony& colony,
                             Coord where ) {
  maybe<DraggableObjectWithBounds<ColViewObject>> o =
      colview_top_level().object_here( where );
  if( !o.has_value() ) return;
  // Could be a unit.
  maybe<Commodity> comm =
      o->obj.get_if<ColViewObject::commodity>().member(
          &ColViewObject::commodity::comm );
  if( !comm.has_value() ) return;

  cheat_increase_commodity( colony, comm->type );
  update_colony_view( ss, colony );
}

void try_decrease_commodity( SS& ss, Colony& colony,
                             Coord where ) {
  maybe<DraggableObjectWithBounds<ColViewObject>> o =
      colview_top_level().object_here( where );
  if( !o.has_value() ) return;
  // Could be a unit.
  maybe<Commodity> comm =
      o->obj.get_if<ColViewObject::commodity>().member(
          &ColViewObject::commodity::comm );
  if( !comm.has_value() ) return;

  cheat_decrease_commodity( colony, comm->type );
  update_colony_view( ss, colony );
}

/****************************************************************
** Colony IPlane
*****************************************************************/
struct ColonyPlane : public IPlane {
  IEngine& engine_;
  SS& ss_;
  TS& ts_;
  Player& player_;
  Colony& colony_;

  ColonyId colony_id_               = {};
  co::stream<input::event_t> input_ = {};
  // These need to be tracked on a separate stream so that we can
  // receive them and interrupt the coroutine that is servicing
  // the input queue. This is because when we recomposite we need
  // destroy all of the existing views, which would in generally
  // leave some coroutines running on dangling pointers to ob-
  // jects.
  co::stream<e_resolution> recomposite_       = {};
  maybe<DragState<ColViewObject>> drag_state_ = {};

  ColonyPlane( IEngine& engine, SS& ss, TS& ts, Colony& colony )
    : engine_( engine ),
      ss_( ss ),
      ts_( ts ),
      player_( ss.players.players[colony.player].value() ),
      colony_( colony ) {}

  void on_logical_resolution_selected(
      e_resolution const resolution ) override {
    // Can't do the recomposite immediately in this function
    // since we need to ensure that any coroutines that are run-
    // ning on top of it get cancelled first.
    recomposite_.send( resolution );
  }

  void draw( rr::Renderer& renderer ) const override {
    draw_colony_view( engine_, colony_, renderer );
    if( drag_state_.has_value() )
      colview_drag_n_drop_draw( ss_, renderer, *drag_state_,
                                point{} );
  }

  e_input_handled input( input::event_t const& event ) override {
    input_.send( event );
    return e_input_handled::yes;
  }

  IPlane::e_accept_drag can_drag(
      input::e_mouse_button /*button*/,
      Coord /*origin*/ ) override {
    if( drag_state_ ) return IPlane::e_accept_drag::swallow;
    return e_accept_drag::yes_but_raw;
  }

  wait<> run_colony_view() {
    while( true ) {
      input::event_t event = co_await input_.next();
      auto [exit, suspended] =
          co_await co::detect_suspend( std::visit(
              LC( handle_event( _ ) ), event.as_base() ) );
      if( suspended ) clear_non_essential_events();
      if( exit ) co_return;
    }
  }

  /**************************************************************
  ** Input Handling
  ***************************************************************/
  // Returns true if the user wants to exit the colony view.
  wait<bool> handle_event( input::key_event_t const& event ) {
    if( event.change != input::e_key_change::down )
      co_return false;
    if( event.mod.shf_down ) {
      // Cheat commands.
      if( !cheat_mode_enabled( ss_ ) ) co_return false;
      switch( event.keycode ) {
        case ::SDLK_1:
          co_await cheat_colony_buildings( colony_, ts_.gui );
          update_colony_view( ss_, colony_ );
          break;
        case ::SDLK_t:
          cheat_create_new_colonist( ss_, ts_, player_,
                                     colony_ );
          update_colony_view( ss_, colony_ );
          break;
        case ::SDLK_SPACE:
          cheat_advance_colony_one_turn(
              RealColonyEvolver( ss_, ts_ ), colony_ );
          update_colony_view( ss_, colony_ );
          break;
        case ::SDLK_LEFTBRACKET:
          if( colony_.hammers > 0 ) --colony_.hammers;
          update_colony_view( ss_, colony_ );
          break;
        case ::SDLK_RIGHTBRACKET:
          ++colony_.hammers;
          update_colony_view( ss_, colony_ );
          break;
        default: //
          break;
      }
    }
    switch( event.keycode ) {
      case ::SDLK_ESCAPE: //
        co_return true;
      case ::SDLK_n:
        ss_.settings.colony_options.numbers =
            !ss_.settings.colony_options.numbers;
        update_colony_view( ss_, colony_ );
        break;
      default: //
        break;
    }
    co_return false;
  }

  // Returns true if the user wants to exit the colony view.
  wait<bool> handle_event(
      input::mouse_button_event_t const& event ) {
    // Need to filter these out otherwise the start of drag
    // events will call perform_click which we don't want.
    if( event.buttons != input::e_mouse_button_event::left_up &&
        event.buttons != input::e_mouse_button_event::right_up )
      co_return false;
    if( event.mod.shf_down ) {
      // Cheat commands.
      if( !cheat_mode_enabled( ss_ ) ) co_return false;
      switch( event.buttons ) {
        case input::e_mouse_button_event::left_up:
          try_promote_demote_unit( ss_, ts_, colony_, event.pos,
                                   /*demote=*/false );
          try_increase_commodity( ss_, colony_, event.pos );
          break;
        case input::e_mouse_button_event::right_up:
          try_promote_demote_unit( ss_, ts_, colony_, event.pos,
                                   /*demote=*/true );
          try_decrease_commodity( ss_, colony_, event.pos );
          break;
        default:
          break;
      }
      co_return false;
    }
    co_await colview_top_level().perform_click( event );
    update_colony_view( ss_, colony_ );
    co_return false;
  }

  wait<bool> handle_event(
      input::mouse_drag_event_t const& event ) {
    co_await drag_drop_routine( input_, colview_top_level(),
                                drag_state_, ts_.gui, event );
    update_colony_view( ss_, colony_ );
    co_return false;
  }

  wait<bool> handle_event( auto const& ) { co_return false; }

  // Remove all input events from the queue corresponding to
  // normal user input, but save the ones that we always need to
  // process, such as window resize events (which are needed to
  // maintain proper drawing as the window is resized).
  void clear_non_essential_events() {
    vector<input::event_t> saved;
    while( input_.ready() ) {
      input::event_t e = input_.next().get();
      switch( e.to_enum() ) {
        case input::e_input_event::win_event:
          saved.push_back( std::move( e ) );
          break;
        default:
          break;
      }
    }
    CHECK( !input_.ready() );
    // Re-insert the ones we want to save.
    for( input::event_t& e : saved )
      input_.send( std::move( e ) );
  }

  wait<> show_colony_view( Colony const& colony ) {
    lg.info( "viewing colony '{}'.", colony.name );

    // This is the only place where this should run; it guaran-
    // tees that there is no coroutine running that reaches into
    // the child views, because if there were and we do a recom-
    // posite then we'd have a running coroutine with dangling
    // pointers to views.
    auto const recomposite = [&] {
      set_colview_colony( engine_, ss_, ts_, player_, colony_ );
    };
    recomposite();

    // Recomposite loop.
    while( true ) {
      auto const next = co_await co::first( recomposite_.next(),
                                            run_colony_view() );
      if( !next.holds<e_resolution>() ) break;
      recomposite();
    }

    lg.info( "leaving colony view." );
  }
};

} // namespace

/****************************************************************
** ColonyViewer
*****************************************************************/
ColonyViewer::ColonyViewer( IEngine& engine, SS& ss )
  : engine_( engine ), ss_( ss ) {}

wait<> ColonyViewer::show_impl( TS& ts, Colony& colony ) {
  Planes& planes        = ts.planes;
  auto owner            = planes.push();
  PlaneGroup& new_group = owner.group;

  ColonyPlane colony_plane( engine_, ss_, ts, colony );
  new_group.bottom = &colony_plane;

  co_await colony_plane.show_colony_view( colony );
}

wait<e_colony_abandoned> ColonyViewer::show(
    TS& ts, ColonyId colony_id ) {
  Colony& colony = ss_.colonies.colony_for( colony_id );
  try {
    co_await show_impl( ts, colony );
    co_return e_colony_abandoned::no;
  } catch( colony_abandon_interrupt const& ) {}

  // We are abandoned.
  co_await run_animated_colony_destruction(
      ss_, ts, colony, e_ship_damaged_reason::colony_abandoned,
      /*msg=*/nothing );
  co_return e_colony_abandoned::yes;
}

} // namespace rn
