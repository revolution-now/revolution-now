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
#include "anim.hpp"
#include "cheat.hpp"
#include "co-combinator.hpp"
#include "colony-mgr.hpp"
#include "colony.hpp"
#include "colview-entities.hpp"
#include "compositor.hpp"
#include "dragdrop.hpp"
#include "gui.hpp"
#include "interrupts.hpp"
#include "logger.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "text.hpp"
#include "ts.hpp"
#include "window.hpp"

// ss
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// render
#include "render/renderer.hpp"

// refl
#include "refl/to-str.hpp"

// gfx
#include "gfx/pixel.hpp"

// base
#include "base/lambda.hpp"

using namespace std;

namespace rn {

namespace {

struct IMapUpdater;

/****************************************************************
** Drawing
*****************************************************************/
void draw_colony_view( Colony const&, rr::Renderer& renderer ) {
  static gfx::pixel background_color =
      gfx::pixel::parse_from_hex( "f1cf81" ).value();
  renderer.painter().draw_solid_rect(
      gfx::rect{ .origin = {},
                 .size   = renderer.logical_screen_size() },
      background_color );

  UNWRAP_CHECK( canvas, compositor::section(
                            compositor::e_section::normal ) );

  colview_top_level().view().draw( renderer,
                                   canvas.upper_left() );
}

/****************************************************************
** Cheat Stuff
*****************************************************************/
void try_promote_demote_unit( SS& ss, Colony& colony,
                              Coord where, bool demote ) {
  maybe<DraggableObjectWithBounds<ColViewObject_t>> o =
      colview_top_level().object_here( where );
  if( !o.has_value() ) return;
  // Could be a commodity.
  maybe<UnitId> unit_id =
      o->obj.get_if<ColViewObject::unit>().member(
          &ColViewObject::unit::id );
  if( !unit_id.has_value() ) return;

  Unit& unit = ss.units.unit_for( *unit_id );
  if( demote )
    cheat_downgrade_unit_expertise( unit );
  else
    cheat_upgrade_unit_expertise( ss, unit );
  update_colony_view( ss, colony );
}

void try_increase_commodity( SS& ss, Colony& colony,
                             Coord where ) {
  maybe<DraggableObjectWithBounds<ColViewObject_t>> o =
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
  maybe<DraggableObjectWithBounds<ColViewObject_t>> o =
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

} // namespace

/****************************************************************
** Colony Plane
*****************************************************************/
struct ColonyPlane::Impl : public Plane {
  SS&     ss_;
  TS&     ts_;
  Player& player_;
  Colony& colony_;

  ColonyId                          colony_id_  = {};
  co::stream<input::event_t>        input_      = {};
  maybe<DragState<ColViewObject_t>> drag_state_ = {};

  Impl( SS& ss, TS& ts, Colony& colony )
    : ss_( ss ),
      ts_( ts ),
      player_( ss.players.players[colony.nation].value() ),
      colony_( colony ) {
    set_colview_colony( ss_, ts_, colony_ );
  }

  bool covers_screen() const override { return true; }

  // FIXME: find a better way to do this. One idea is that when
  // the compositor changes the layout it will inject a window
  // resize event into the input queue that will then be automat-
  // ically picked up by all of the planes.
  void advance_state() override {
    UNWRAP_CHECK(
        new_canvas,
        compositor::section( compositor::e_section::normal ) );
    if( new_canvas != canvas_ ) {
      canvas_ = new_canvas;
      // This is slightly hacky since this is not a real window
      // resize event, but it'll do for now. Doing it this way
      // will ensure that 1) we wake up the input-processing
      // coroutine which is likely asleep waiting for input
      // events, and 2) we allow the same logic to handle this
      // recompositing that is used for real window resize
      // events, which is good because it has some safeguards
      // built in to it, such as not allowing a recomposite
      // during a drag operation (which would cause dangling
      // pointers; see the comment about that in the dragging
      // coroutine. This should probably be fixed).
      input_.send( input::win_event_t{
          input::event_base_t{},
          /*type=*/input::e_win_event_type::resized } );
    }
  }

  void draw( rr::Renderer& renderer ) const override {
    draw_colony_view( colony_, renderer );
    if( drag_state_.has_value() )
      colview_drag_n_drop_draw( ss_, renderer, *drag_state_,
                                canvas_.upper_left() );
  }

  e_input_handled input( input::event_t const& event ) override {
    input::event_t event_translated = move_mouse_origin_by(
        event, canvas_.upper_left().distance_from_origin() );
    input_.send( event_translated );
    if( event_translated.holds<input::win_event_t>() )
      // Generally we should return no here because this is an
      // event that we want all planes to see. FIXME: need to
      // find a better way to handle this automatically.
      return e_input_handled::no;
    return e_input_handled::yes;
  }

  Plane::e_accept_drag can_drag(
      input::e_mouse_button /*button*/,
      Coord /*origin*/ ) override {
    if( drag_state_ ) return Plane::e_accept_drag::swallow;
    return e_accept_drag::yes_but_raw;
  }

  wait<> run_colview() {
    while( true ) {
      input::event_t event   = co_await input_.next();
      auto [exit, suspended] = co_await co::detect_suspend(
          std::visit( LC( handle_event( _ ) ), event ) );
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
      switch( event.keycode ) {
        case ::SDLK_1:
          co_await cheat_colony_buildings( colony_, ts_.gui );
          update_colony_view( ss_, colony_ );
          break;
        case ::SDLK_t:
          cheat_create_new_colonist( ss_, ts_, colony_ );
          update_colony_view( ss_, colony_ );
          break;
        case ::SDLK_SPACE:
          cheat_advance_colony_one_turn( ss_, ts_, colony_ );
          update_colony_view( ss_, colony_ );
          break;
        default: //
          break;
      }
    }
    switch( event.keycode ) {
      case ::SDLK_ESCAPE: //
        co_return true;
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
      switch( event.buttons ) {
        case input::e_mouse_button_event::left_up:
          try_promote_demote_unit( ss_, colony_, event.pos,
                                   /*demote=*/false );
          try_increase_commodity( ss_, colony_, event.pos );
          break;
        case input::e_mouse_button_event::right_up:
          try_promote_demote_unit( ss_, colony_, event.pos,
                                   /*demote=*/true );
          try_decrease_commodity( ss_, colony_, event.pos );
          break;
        default: break;
      }
      co_return false;
    }
    co_await colview_top_level().perform_click( event );
    co_return false;
  }

  wait<bool> handle_event( input::win_event_t const& event ) {
    if( event.type == input::e_win_event_type::resized )
      // Force a re-composite.
      set_colview_colony( ss_, ts_, colony_ );
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
        default: break;
      }
    }
    CHECK( !input_.ready() );
    // Re-insert the ones we want to save.
    for( input::event_t& e : saved )
      input_.send( std::move( e ) );
  }

  Rect canvas_;
};

namespace {

wait<> show_colony_view_impl( Planes& planes, SS& ss, TS& ts_old,
                              Colony& colony ) {
  PlaneGroup const& old_group = planes.back();
  auto              popper    = planes.new_group();
  PlaneGroup&       new_group = planes.back();

  new_group.omni    = old_group.omni;
  new_group.console = old_group.console;

  WindowPlane window_plane;
  new_group.window = &window_plane;

  RealGui gui( window_plane );

  TS ts( ts_old.map_updater, ts_old.lua, gui, ts_old.rand );

  ColonyPlane colony_plane( ss, ts, colony );
  new_group.colony = &colony_plane;

  lg.info( "viewing colony '{}'.", colony.name );
  co_await colony_plane.show_colony_view();
  lg.info( "leaving colony view." );
}

} // namespace

/****************************************************************
** ColonyPlane
*****************************************************************/
Plane& ColonyPlane::impl() { return *impl_; }

ColonyPlane::~ColonyPlane() = default;

ColonyPlane::ColonyPlane( SS& ss, TS& ts, Colony& colony )
  : impl_( new Impl( ss, ts, colony ) ) {}

wait<> ColonyPlane::show_colony_view() const {
  co_await impl_->run_colview();
}

/****************************************************************
** API
*****************************************************************/
wait<base::NoDiscard<e_colony_abandoned>> show_colony_view(
    Planes& planes, SS& ss, TS& ts, Colony& colony ) {
  try {
    co_await show_colony_view_impl( planes, ss, ts, colony );
    co_return e_colony_abandoned::no;
  } catch( colony_abandon_interrupt const& ) {}

  // We are abandoned.
  co_await run_colony_destruction( planes, ss, ts, colony,
                                   /*msg=*/nothing );
  co_return e_colony_abandoned::yes;
}

} // namespace rn
