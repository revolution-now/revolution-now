/****************************************************************
**harbor-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-08.
*
* Description: The european harbor view.
*
*****************************************************************/
#include "harbor-view.hpp"

// Revolution Now
#include "cheat.hpp"
#include "co-combinator.hpp"
#include "commodity.hpp"
#include "drag-drop.hpp"
#include "harbor-view-entities.hpp"
#include "iengine.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "player-mgr.hpp"
#include "render.hpp"
#include "screen.hpp" // FIXME: remove
#include "ts.hpp"
#include "view.hpp"

// config
#include "config/nation.hpp"
#include "config/text.rds.hpp"

// render
#include "render/renderer.hpp"

// ss
#include "ss/old-world-state.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/turn.rds.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/resolution-enum.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/lambda.hpp"
#include "base/logger.hpp"

// C++ standard library
#include <exception>

namespace rn {
namespace {

using namespace std;

using ::gfx::e_resolution;
using ::gfx::point;
using ::gfx::size;

void check_selected_unit_in_harbor( SSConst const& ss,
                                    Player const& player ) {
  // In case the unit that was selected
  HarborState const& hb_state =
      old_world_state( ss, player.type ).harbor_state;
  if( !hb_state.selected_unit.has_value() ) return;
  UnitId id = *hb_state.selected_unit;
  CHECK( ss.units.exists( id ) );
  CHECK( ss.units.maybe_harbor_view_state_of( id ) );
}

/****************************************************************
** Harbor IPlane
*****************************************************************/
struct HarborPlane : public IPlane {
  IEngine& engine_;
  SS& ss_;
  TS& ts_;
  Player& player_;

  co::stream<input::event_t> input_ = {};
  // These need to be tracked on a separate stream so that we can
  // receive them and interrupt the coroutine that is servicing
  // the input queue. This is because when we recomposite we need
  // destroy all of the existing views, which would in generally
  // leave some coroutines running on dangling pointers to ob-
  // jects.
  co::stream<e_resolution> recomposite_               = {};
  maybe<DragState<HarborDraggableObject>> drag_state_ = {};

  HarborViewComposited composition_;

  HarborPlane( IEngine& engine, SS& ss, TS& ts, Player& player )
    : engine_( engine ),
      ss_( ss ),
      ts_( ts ),
      player_( player ) {}

  void on_logical_resolution_selected(
      e_resolution const resolution ) override {
    // Can't do the recomposite immediately in this function
    // since we need to ensure that any coroutines that are run-
    // ning on top of it get cancelled first.
    recomposite_.send( resolution );
  }

  HarborSubView& harbor_view_top_level() const {
    CHECK( composition_.top_level != nullptr );
    return *composition_.top_level;
  }

  wait<> clear_status_bar_msg() const {
    // Just a hack to tell the status bar to revert back to a de-
    // fault message. This allows us to replicate the OG's be-
    // havior which has a good feel in that a click anywhere im-
    // mediately creates a reponse that clears the status bar and
    // returns the text to the default.
    co_await composition_
        .entities[e_harbor_view_entity::status_bar]
        ->perform_click( input::mouse_button_event_t() );
  }

  void draw( rr::Renderer& renderer, Coord ) const override {
    harbor_view_top_level().view().draw( renderer, point{} );
    harbor_view_drag_n_drop_draw( renderer );
  }

  void harbor_view_drag_n_drop_draw(
      rr::Renderer& renderer ) const {
    if( !drag_state_.has_value() ) return;
    DragState<HarborDraggableObject> const& state = *drag_state_;
    Coord const canvas_origin;
    Coord const sprite_upper_left =
        state.where - state.click_offset +
        canvas_origin.distance_from_origin();
    // Render the dragged item.
    overload_visit(
        state.object,
        [&]( HarborDraggableObject::unit const& o ) {
          render_unit( renderer, sprite_upper_left,
                       ss_.units.unit_for( o.id ),
                       UnitRenderOptions{} );
        },
        [&]( HarborDraggableObject::market_commodity const& o ) {
          render_commodity_20( renderer, sprite_upper_left,
                               o.comm.type );
        },
        [&]( HarborDraggableObject::cargo_commodity const& o ) {
          render_commodity_20(
              renderer,
              sprite_upper_left.to_gfx() +
                  kLabeledCommodity20CargoRenderOffset,
              o.comm.type );
        } );
    // Render any indicators on top of it.
    switch( state.indicator ) {
      using e = e_drag_status_indicator;
      case e::none:
        break;
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
        if( state.source_requests_edit ) {
          auto mod_pos = state.where;
          mod_pos.y -= H{ typer.dimensions_for_line( "?" ).h };
          mod_pos -= state.click_offset;
          auto typer_mod =
              renderer.typer( mod_pos, gfx::pixel::green() );
          typer_mod.write( "?" );
        }
        break;
      }
    }
  }

  bool on_input( input::event_t const& event ) override {
    input_.send( event );
    return true;
  }

  IPlane::e_accept_drag can_drag(
      input::e_mouse_button /*button*/,
      Coord /*origin*/ ) override {
    if( drag_state_ ) return IPlane::e_accept_drag::swallow;
    return e_accept_drag::yes_but_raw;
  }

  wait<> run_harbor_view() {
    CHECK( composition_.top_level != nullptr );
    while( true ) {
      input::event_t event = co_await input_.next();
      auto [ignored, suspended] =
          co_await co::detect_suspend( std::visit(
              LC( handle_event( _ ) ), event.as_base() ) );
      if( suspended ) clear_non_essential_events();
    }
  }

  /**************************************************************
  ** Input Handling
  ***************************************************************/
  // Returns true if the user wants to exit the colony view.
  wait<> handle_event( input::key_event_t const& event ) {
    if( event.change != input::e_key_change::down ) co_return;

    // Intercept cheat commands.
    if( event.mod.shf_down ) {
      if( !cheat_mode_enabled( ss_ ) ) co_return;
      switch( event.keycode ) {
        case ::SDLK_LEFTBRACKET:
          cheat_decrease_tax_rate( ss_, player_ );
          break;
        case ::SDLK_RIGHTBRACKET:
          cheat_increase_tax_rate( ss_, player_ );
          break;
        case ::SDLK_3: //
          cheat_decrease_gold( player_ );
          break;
        case ::SDLK_4: //
          cheat_increase_gold( player_ );
          break;
        case ::SDLK_SPACE: //
          co_await cheat_evolve_market_prices( ss_, ts_,
                                               player_ );
          break;
      }
      co_return;
    }

    // Intercept harbor view global commands.
    switch( event.keycode ) {
      case ::SDLK_ESCAPE: //
      case ::SDLK_e:      //
        throw harbor_view_exit_interrupt{};
      default: //
        break;
    }

    // Try the no-coro handler.
    bool handled = harbor_view_top_level().view().input( event );
    if( handled ) co_return;

    // Finally fall back to the coroutine key handler.
    co_await harbor_view_top_level().perform_key( event );
  }

  // Returns true if the user wants to exit the colony view.
  wait<> handle_event(
      input::mouse_button_event_t const& event ) {
    // In the harbor view we need to use up-clicks because gener-
    // ally entities support both clicking and dragging, and we
    // don't want the down click on a drag to call perform_click.
    if( event.buttons != input::e_mouse_button_event::left_up &&
        event.buttons != input::e_mouse_button_event::right_up )
      co_return;
    if( event.mod.shf_down ) {
      // Cheat commands.
      // TODO

      // If not handled then fall through, since some cheat com-
      // mands are implemented in the individual views.
    }
    // The OG has this nice UI behavior where clicking anywhere
    // will immediately remove any transient status bar messages
    // back to the default status bar, which is nice.
    co_await clear_status_bar_msg();
    co_await harbor_view_top_level().perform_click( event );
  }

  wait<> handle_event( input::mouse_drag_event_t const& event ) {
    co_await drag_drop_routine( input_, harbor_view_top_level(),
                                drag_state_, ts_.gui, event );
  }

  // The `auto` here is intended to represent the variants of
  // input::event_t that are not captured above.
  wait<> handle_event( auto const& e ) {
    [[maybe_unused]] bool const handled =
        harbor_view_top_level().view().input( e );
    co_return;
  }

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

  wait<> show_harbor_view() {
    lg.info( "entering harbor view." );

    // This is the only place where this should run; it guaran-
    // tees that there is no coroutine running that reaches into
    // the child views, because if there were and we do a recom-
    // posite then we'd have a running coroutine with dangling
    // pointers to views.
    auto const recomposite = [&]( e_resolution const r ) {
      composition_ =
          recomposite_harbor_view( ss_, ts_, player_, r );
    };
    CHECK( !composition_.top_level );
    recomposite(
        named_resolution( engine_ ).value_or( e_resolution{} ) );
    CHECK( composition_.top_level != nullptr );

    while( true ) {
      auto const next = co_await co::first( recomposite_.next(),
                                            run_harbor_view() );
      auto const resolution = next.get_if<e_resolution>();
      if( !resolution ) break;
      recomposite( *resolution );
    }

    lg.info( "leaving harbor view." );
  }
};

} // namespace

/****************************************************************
** HarborPlane
*****************************************************************/
HarborViewer::HarborViewer( IEngine& engine, SS& ss, TS& ts,
                            Player& player )
  : engine_( engine ), ss_( ss ), ts_( ts ), player_( player ) {}

void HarborViewer::set_selected_unit( UnitId const id ) {
  UnitsState const& units_state = ss_.units;
  // Ensure that the unit is either in port or on the high
  // seas, otherwise it doesn't make sense for the unit to be
  // selected on this screen.
  CHECK( units_state.maybe_harbor_view_state_of( id ) );
  HarborState& hb_state =
      old_world_state( ss_, player_.type ).harbor_state;
  hb_state.selected_unit = id;
}

wait<> HarborViewer::show() {
  Planes& planes    = ts_.planes;
  auto owner        = planes.push();
  PlaneGroup& group = owner.group;

  HarborPlane harbor_plane( engine_, ss_, ts_, player_ );
  check_selected_unit_in_harbor( ss_, player_ );
  group.bottom = &harbor_plane;
  try {
    // This coroutine should never return but by throwing the
    // exit exception.
    co_await harbor_plane.show_harbor_view();
  } catch( harbor_view_exit_interrupt const& ) {}
}

} // namespace rn
