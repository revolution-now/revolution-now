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
#include "co-combinator.hpp"
#include "color.hpp"
#include "colview-entities.hpp"
#include "compositor.hpp"
#include "cstate.hpp"
#include "logging.hpp"
#include "plane-ctrl.hpp"
#include "plane.hpp"
#include "text.hpp"

// base
#include "base/lambda.hpp"

using namespace std;

namespace rn {

namespace {

using e_input_handled = Plane::e_input_handled;

/****************************************************************
** Globals
*****************************************************************/
ColonyId                   g_colony_id{};
co::stream<input::event_t> g_input;

/****************************************************************
** Drawing
*****************************************************************/
void draw_colony_view( Texture& tx, ColonyId id ) {
  tx.fill( Color::parse_from_hex( "f1cf81" ).value() );

  UNWRAP_CHECK( canvas,
                compositor::section(
                    compositor::e_section::non_menu_bar ) );

  auto& colony = colony_from_id( id );

  Coord pos = canvas.upper_left();

  auto line = [&]( string_view fmt_str, auto&&... args ) {
    string text = fmt::format( fmt_str, args... );
    render_text( font::standard(), Color::black(), text )
        .copy_to( tx, pos );
    pos += 16_h;
  };

  line( "" );
  line( "id: {}", colony.id() );
  line( "nation: {}", colony.nation() );
  line( "location: {}", colony.location() );

  colview_top_level().view().draw( tx, Coord{} );
}

/****************************************************************
** Input Handling
*****************************************************************/
// Returns true if the user wants to exit the colony view.
waitable<bool> handle_event( input::key_event_t const& event ) {
  if( event.change != input::e_key_change::down )
    co_return false;
  switch( event.keycode ) {
    case ::SDLK_ESCAPE: //
      co_return true;
    default: //
      break;
  }
  co_return false;
}

// Returns true if the user wants to exit the colony view.
waitable<bool> handle_event(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return false;
  Coord click_pos = event.pos;
  co_await colview_top_level().perform_click( click_pos );
  co_return false;
}

waitable<bool> handle_event( input::win_event_t const& event ) {
  if( event.type == input::e_win_event_type::resized )
    // Force a re-composite.
    set_colview_colony( g_colony_id );
  co_return false;
}

waitable<bool> handle_event(
    input::mouse_drag_event_t const& event ) {
  CHECK( event.state.phase == input::e_drag_phase::begin );
  Coord const& origin = event.state.origin;
  while( true ) {
    input::event_t event = co_await g_input.next();
    if( !event.holds<input::mouse_drag_event_t>() ) continue;
    input::mouse_drag_event_t const& drag_event =
        event.get<input::mouse_drag_event_t>();
  }
}

waitable<bool> handle_event( auto const& ) { co_return false; }

waitable<> run_colview() {
  while( true ) {
    input::event_t event   = co_await g_input.next();
    auto [exit, suspended] = co_await co::detect_suspend(
        std::visit( L( handle_event( _ ) ), event ) );
    if( suspended ) g_input.reset();
    if( exit ) co_return;
  }
}

/****************************************************************
** Colony Plane
*****************************************************************/
struct ColonyPlane : public Plane {
  ColonyPlane() = default;
  bool covers_screen() const override { return true; }
  void draw( Texture& tx ) const override {
    draw_colony_view( tx, g_colony_id );
  }
  e_input_handled input( input::event_t const& event ) override {
    g_input.send( event );
    if( event.holds<input::win_event_t>() )
      // Generally we should return no here because this is an
      // event that we want all planes to see. FIXME: need to
      // find a better way to handle this automatically.
      return e_input_handled::no;
    return e_input_handled::yes;
  }
};

ColonyPlane g_colony_plane;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
Plane* colony_plane() { return &g_colony_plane; }

waitable<> show_colony_view( ColonyId id ) {
  CHECK( colony_exists( id ) );
  g_input.reset();
  g_colony_id = id;
  set_colview_colony( id );
  ScopedPlanePush pusher( e_plane_config::colony );
  lg.info( "viewing colony {}.", colony_from_id( id ) );
  co_await run_colview();
  lg.info( "leaving colony view." );
}

} // namespace rn
