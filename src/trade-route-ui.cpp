/****************************************************************
**trade-route-ui.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Shows the Trade Route editing UI screen.
*
*****************************************************************/
#include "trade-route-ui.hpp"

// rds
#include "trade-route-ui-impl.rds.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "input.hpp"
#include "plane-stack.hpp"
#include "screen.hpp"

// ss
#include "gfx/resolution-enum.rds.hpp"

// render
#include "render/painter.hpp"
#include "render/renderer.hpp"
#include "render/typer.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/pixel.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::e_resolution;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

/****************************************************************
** Layout.
*****************************************************************/
struct Layout {
  e_resolution named_resolution = {};

  // TODO
};

/****************************************************************
** Auto-Layout.
*****************************************************************/
Layout layout_auto( e_resolution const ) {
  Layout l;

  // TODO

  return l;
}

/****************************************************************
** Layouts.
*****************************************************************/
// Example for how to customize for a particular resolution.
Layout layout_576x360() {
  Layout l = layout_auto( e_resolution::_576x360 );
  // Customize.
  return l;
}

/****************************************************************
** TradeRouteUI
*****************************************************************/
struct TradeRouteUI : public IPlane {
  // Doing private by default allows the compiler to tell us
  // which methods aren't used.

 private:
  // State
  IEngine& engine_;
  SSConst const& ss_;
  Player const& player_;
  TradeRouteState& trade_route_state_;
  maybe<Layout> layout_    = {};
  wait_promise<> finished_ = {};
  co::stream<TradeRouteInput> in_;
  TradeRouteId curr_id_ = {};

 public:
  TradeRouteUI( IEngine& engine, SSConst const& ss,
                Player const& player,
                TradeRouteState& trade_route_state,
                TradeRouteId const initial_id )
    : engine_( engine ),
      ss_( ss ),
      player_( player ),
      trade_route_state_( trade_route_state ),
      curr_id_( initial_id ) {
    if( auto const named = named_resolution( engine_ );
        named.has_value() )
      on_logical_resolution_changed( *named );
  }

  // Doing private by default allows the compiler to tell us
  // which methods aren't used.

 private:
  Layout layout_gen( e_resolution const resolution ) {
    using enum e_resolution;
    switch( resolution ) {
      case _576x360:
        // Example for how to customize per resolution.
        return layout_576x360();
      default:
        return layout_auto( resolution );
    }
  }

  void on_logical_resolution_selected(
      e_resolution const resolution ) override {
    layout_ = layout_gen( resolution );
  }

  void write_centered( rr::Renderer& renderer,
                       pixel const color_fg,
                       maybe<pixel> const color_bg,
                       point const center,
                       string_view const text ) const {
    rr::Typer typer      = renderer.typer();
    size const text_size = typer.dimensions_for_line( text );
    rect const text_rect = gfx::centered_on( text_size, center );
    if( color_bg.has_value() ) {
      typer.set_color( *color_bg );
      typer.set_position( text_rect.nw() + size{ .w = 1 } );
      typer.write( text );
      typer.set_position( text_rect.nw() + size{ .h = 1 } );
      typer.write( text );
    }
    typer.set_color( color_fg );
    typer.set_position( text_rect.nw() );
    typer.write( text );
  }

  void draw( rr::Renderer& renderer, const Layout& l ) const {
    rr::Painter painter = renderer.painter();
    painter.draw_solid_rect( renderer.logical_screen_rect(),
                             pixel::from_hex_rgb( 0xdddddd ) );

    (void)l;
  }

  void draw( rr::Renderer& renderer ) const override {
    if( !layout_ ) return;
    draw( renderer, *layout_ );
  }

  e_input_handled on_key(
      input::key_event_t const& event ) override {
    if( event.change != input::e_key_change::down )
      return e_input_handled::no;
    if( input::is_mod_key( event ) ) return e_input_handled::no;
    finished_.set_value( monostate{} );
    return e_input_handled::yes;
  }

  e_input_handled on_mouse_button(
      input::mouse_button_event_t const& event ) override {
    // Need to use left_up here otherwise the down click will
    // close this screen and then the up click will get picked up
    // by the land-view which requires using the up click in
    // order to distinguish clicks from drags.
    if( event.buttons != input::e_mouse_button_event::left_up )
      return e_input_handled::no;
    finished_.set_value( monostate{} );
    return e_input_handled::yes;
  }

 public:
  wait<> run() { co_await finished_.wait(); }
};

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
wait<> show_trade_route_edit_ui(
    IEngine& engine, SSConst const& ss, Player const& player,
    IGui& gui, Planes& planes,
    TradeRouteState& trade_route_state,
    TradeRouteId const trade_route_id ) {
  auto owner        = planes.push();
  PlaneGroup& group = owner.group;
  TradeRouteUI trade_route_ui(
      engine, ss, player, trade_route_state, trade_route_id );
  group.bottom = &trade_route_ui;
  co_await trade_route_ui.run();
}

} // namespace rn
