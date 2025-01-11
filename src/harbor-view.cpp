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
#include "commodity.hpp"
#include "drag-drop.hpp"
#include "harbor-view-entities.hpp"
#include "iengine.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
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

using ::gfx::point;

void check_selected_unit_in_harbor( SSConst const& ss,
                                    Player const& player ) {
  // In case the unit that was selected
  HarborState const& hb_state = player.old_world.harbor_state;
  if( !hb_state.selected_unit.has_value() ) return;
  UnitId id = *hb_state.selected_unit;
  CHECK( ss.units.exists( id ) );
  CHECK( ss.units.maybe_harbor_view_state_of( id ) );
}

} // namespace

/****************************************************************
** Harbor IPlane
*****************************************************************/
struct HarborPlane::Impl : public IPlane {
  IEngine& engine_;
  SS& ss_;
  TS& ts_;
  Player& player_;

  co::stream<input::event_t> input_                   = {};
  maybe<DragState<HarborDraggableObject>> drag_state_ = {};

  HarborViewComposited composition_;

  Impl( IEngine& engine, SS& ss, TS& ts, Player& player )
    : engine_( engine ),
      ss_( ss ),
      ts_( ts ),
      player_( player ) {
    auto const new_canvas = main_window_logical_rect(
        engine_.video(), engine_.window(),
        engine_.resolutions() );
    composition_ = recomposite_harbor_view( ss_, ts_, player_,
                                            new_canvas.size );
  }

  void on_logical_resolution_selected(
      gfx::e_resolution const resolution ) override {
    composition_ = recomposite_harbor_view(
        ss_, ts_, player_, resolution_size( resolution ) );
  }

  HarborState& harbor_state() {
    return player_.old_world.harbor_state;
  }

  HarborSubView& harbor_view_top_level() const {
    CHECK( composition_.top_level != nullptr );
    return *composition_.top_level;
  }

  void draw( rr::Renderer& renderer ) const override {
    harbor_view_top_level().view().draw( renderer, point{} );
    harbor_view_drag_n_drop_draw( renderer );
    draw_stats( renderer );
  }

  void draw_stats( rr::Renderer& renderer ) const {
    auto const canvas = main_window_logical_rect(
        engine_.video(), engine_.window(),
        engine_.resolutions() );
    auto& nation       = nation_obj( player_.nation );
    string const stats = fmt::format(
        "{}, {}. {}, {}. Tax: {}%  Treasury: {}{}",
        nation.harbor_city_name, nation.country_name,
        // FIXME
        ts_.gui.identifier_to_display_name(
            refl::enum_value_name(
                ss_.turn.time_point.season ) ),
        ss_.turn.time_point.year,
        player_.old_world.taxes.tax_rate, player_.money,
        config_text.special_chars.currency );
    Coord start = canvas.center();
    start.y     = 1;
    start.x -= rr::rendered_text_line_size_pixels( stats ).w / 2;
    rr::Typer typer =
        renderer.typer( start, gfx::pixel::banana() );
    typer.write( stats );
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
          render_commodity_20( renderer, sprite_upper_left,
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
    input_.send( event );
    return e_input_handled::yes;
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
          cheat_decrease_tax_rate( player_ );
          break;
        case ::SDLK_RIGHTBRACKET:
          cheat_increase_tax_rate( player_ );
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
    // Need to filter these out otherwise the start of drag
    // events will call perform_click which we don't want.
    if( event.buttons != input::e_mouse_button_event::left_up &&
        event.buttons != input::e_mouse_button_event::right_up )
      co_return;
    if( event.mod.shf_down ) {
      // Cheat commands.
      // TODO

      // If not handled then fall through, since some cheat com-
      // mands are implemented in the individual views.
    }
    co_await harbor_view_top_level().perform_click( event );
  }

  wait<> handle_event( input::mouse_drag_event_t const& event ) {
    co_await drag_drop_routine( input_, harbor_view_top_level(),
                                drag_state_, ts_.gui, event );
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
** HarborPlane
*****************************************************************/
IPlane& HarborPlane::impl() { return *impl_; }

HarborPlane::~HarborPlane() = default;

HarborPlane::HarborPlane( IEngine& engine, SS& ss, TS& ts,
                          Player& player )
  : impl_( new Impl( engine, ss, ts, player ) ) {}

void HarborPlane::set_selected_unit( UnitId id ) {
  impl_->set_selected_unit( id );
}

wait<> HarborPlane::show_harbor_view() {
  return impl_->show_harbor_view();
}

/****************************************************************
** API
*****************************************************************/
wait<> show_harbor_view( IEngine& engine, SS& ss, TS& ts,
                         Player& player,
                         maybe<UnitId> selected_unit ) {
  Planes& planes    = ts.planes;
  auto owner        = planes.push();
  PlaneGroup& group = owner.group;

  HarborPlane harbor_plane( engine, ss, ts, player );
  check_selected_unit_in_harbor( ss, player );
  if( selected_unit.has_value() )
    harbor_plane.set_selected_unit( *selected_unit );
  group.bottom = &harbor_plane.impl();
  try {
    // This coroutine should never return but by throwing the
    // exit exception.
    co_await harbor_plane.show_harbor_view();
  } catch( harbor_view_exit_interrupt const& ) {}
}

} // namespace rn
