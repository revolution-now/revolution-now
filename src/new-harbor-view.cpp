/****************************************************************
**new-harbor-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-08.
*
* Description: The european harbor view.
*
*****************************************************************/
#include "new-harbor-view.hpp"

// Revolution Now
#include "commodity.hpp"
#include "compositor.hpp"
#include "dragdrop.hpp"
#include "harbor-view-entities.hpp"
#include "logger.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "render.hpp"
#include "ts.hpp"
#include "view.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/lambda.hpp"

// C++ standard library
#include <exception>

namespace rn {
namespace {

using namespace std;

} // namespace

/****************************************************************
** Harbor Plane
*****************************************************************/
struct NewHarborPlane::Impl : public Plane {
  SS&     ss_;
  TS&     ts_;
  Player& player_;

  Rect                       canvas_;
  co::stream<input::event_t> input_      = {};
  maybe<DragState>           drag_state_ = {};

  HarborViewComposited composition_;

  Impl( SS& ss, TS& ts, Player& player )
    : ss_( ss ), ts_( ts ), player_( player ) {}

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

  HarborState& harbor_state() {
    return player_.old_world.harbor_state;
  }

  HarborSubView& harbor_view_top_level() const {
    CHECK( composition_.top_level != nullptr );
    return *composition_.top_level;
  }

  void draw( rr::Renderer& renderer ) const override {
    harbor_view_top_level().view().draw( renderer,
                                         canvas_.upper_left() );
    harbor_view_drag_n_drop_draw( renderer );
  }

  void harbor_view_drag_n_drop_draw(
      rr::Renderer& renderer ) const {
    if( !drag_state_.has_value() ) return;
    DragState const& state         = *drag_state_;
    Coord const      canvas_origin = canvas_.upper_left();
    Coord const      sprite_upper_left =
        state.where - state.click_offset +
        canvas_origin.distance_from_origin();
    using namespace HarborDraggableObject;
    // Render the dragged item.
    UNWRAP_DRAGGABLE( object, state.object );
    overload_visit(
        object,
        [&]( unit const& o ) {
          render_unit( renderer, sprite_upper_left,
                       ss_.units.unit_for( o.id ),
                       UnitRenderOptions{ .flag = false } );
        },
        [&]( market_commodity const& o ) {
          render_commodity( renderer, sprite_upper_left,
                            o.type );
        },
        [&]( cargo_commodity const& o ) {
          render_commodity( renderer, sprite_upper_left,
                            o.comm.type );
        } );
    // Render any indicators on top of it.
    switch( state.indicator ) {
      using e = e_drag_status_indicator;
      case e::none: break;
      case e::bad: {
        rr::Typer typer = renderer.typer( sprite_upper_left,
                                          gfx::pixel::red() );
        typer.write( "X" );
        break;
      }
      case e::good: {
        rr::Typer typer = renderer.typer( sprite_upper_left,
                                          gfx::pixel::green() );
        typer.write( "+" );
        if( state.user_requests_input ) {
          auto mod_pos = state.where;
          mod_pos.y -=
              H{ rr::rendered_text_line_size_pixels( "?" ).h };
          mod_pos -= state.click_offset;
          auto typer_mod =
              renderer.typer( mod_pos, gfx::pixel::green() );
          typer_mod.write( "?" );
        }
        break;
      }
    }
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

  wait<> run_harbor_view() {
    while( true ) {
      input::event_t event      = co_await input_.next();
      auto [ignored, suspended] = co_await co::detect_suspend(
          std::visit( LC( handle_event( _ ) ), event ) );
      if( suspended ) clear_non_essential_events();
    }
  }

  /**************************************************************
  ** Input Handling
  ***************************************************************/
  // Returns true if the user wants to exit the colony view.
  wait<> handle_event( input::key_event_t const& event ) {
    if( event.change != input::e_key_change::down ) co_return;
    if( event.mod.shf_down ) {
      // Cheat commands.
      // TODO
      co_return;
    }
    switch( event.keycode ) {
      case ::SDLK_ESCAPE: //
      case ::SDLK_e:      //
        throw harbor_view_exit_interrupt{};
      default: //
        break;
    }
  }

  // Returns true if the user wants to exit the colony view.
  wait<> handle_event(
      input::mouse_button_event_t const& event ) {
    // Need to filter these out otherwise the start of drag
    // events will call perform_click which we don't want.
    if( event.buttons != input::e_mouse_button_event::left_up &&
        event.buttons != input::e_mouse_button_event::right_up )
      co_return;
    if( event.mod.shf_down ) {
      // Cheat commands.
      // TODO
      co_return;
    }
    co_await harbor_view_top_level().perform_click( event );
  }

  wait<> handle_event( input::win_event_t const& event ) {
    if( event.type == input::e_win_event_type::resized )
      composition_ = recomposite_harbor_view( ss_, ts_, player_,
                                              canvas_.delta() );
    co_return;
  }

  wait<> handle_event( input::mouse_drag_event_t const& event ) {
    auto obj_str_func = []( any const& a ) {
      UNWRAP_DRAGGABLE( o, a );
      return fmt::to_string( o );
    };
    co_await drag_drop_routine( input_, harbor_view_top_level(),
                                drag_state_, ts_.gui, event,
                                obj_str_func );
  }

  wait<> handle_event( auto const& ) { co_return; }

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

  wait<> show_harbor_view() {
    HarborState& hb_state = harbor_state();
    if( hb_state.selected_unit ) {
      UnitId id = *hb_state.selected_unit;
      // We could have a case where the unit that was last se-
      // lected went to the new world and was then disbanded, or
      // is just no longer in the harbor.
      if( !ss_.units.exists( id ) ||
          !ss_.units.maybe_harbor_view_state_of( id ) )
        hb_state.selected_unit = nothing;
    }
    lg.info( "entering harbor view." );
    co_await run_harbor_view();
    lg.info( "leaving harbor view." );
  }

  void set_selected_unit( UnitId id ) {
    UnitsState const& units_state = ss_.units;
    // Ensure that the unit is either in port or on the high
    // seas, otherwise it doesn't make sense for the unit to be
    // selected on this screen.
    CHECK( units_state.maybe_harbor_view_state_of( id ) );
    HarborState& hb_state  = harbor_state();
    hb_state.selected_unit = id;
  }
};

/****************************************************************
** NewHarborPlane
*****************************************************************/
Plane& NewHarborPlane::impl() { return *impl_; }

NewHarborPlane::~NewHarborPlane() = default;

NewHarborPlane::NewHarborPlane( SS& ss, TS& ts, Player& player )
  : impl_( new Impl( ss, ts, player ) ) {}

void NewHarborPlane::set_selected_unit( UnitId id ) {
  impl_->set_selected_unit( id );
}

wait<> NewHarborPlane::show_harbor_view() {
  return impl_->show_harbor_view();
}

/****************************************************************
** API
*****************************************************************/
wait<> show_new_harbor_view( Planes& planes, SS& ss, TS& ts,
                             Player&       player,
                             maybe<UnitId> selected_unit ) {
  auto        popper    = planes.new_copied_group();
  PlaneGroup& new_group = planes.back();

  NewHarborPlane harbor_plane( ss, ts, player );
  if( selected_unit.has_value() )
    harbor_plane.set_selected_unit( *selected_unit );
  new_group.new_harbor = &harbor_plane;
  try {
    // This coroutine should never return but by throwing the
    // exit exception.
    co_await harbor_plane.show_harbor_view();
  } catch( harbor_view_exit_interrupt const& ) {}
}

} // namespace rn
