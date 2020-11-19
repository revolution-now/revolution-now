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
#include "color.hpp"
#include "colview-entities.hpp"
#include "compositor.hpp"
#include "cstate.hpp"
#include "menu.hpp"
#include "plane-ctrl.hpp"
#include "plane.hpp"
#include "text.hpp"

using namespace std;

namespace rn {

namespace {

using e_input_handled = Plane::e_input_handled;

void exit_colony_view() { pop_plane_config(); }

void draw_colony_view( Texture& tx, ColonyId id ) {
  tx.fill( Color::parse_from_hex( "f1cf81" ).value() );

  ASSIGN_CHECK_OPT( canvas,
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

  colview_top_level()->draw( tx, Coord{} );
}

e_input_handled handle_key_event(
    input::key_event_t const& key_event ) {
  if( key_event.change != input::e_key_change::down )
    return e_input_handled::no;
  switch( key_event.keycode ) {
    case ::SDLK_ESCAPE: //
      exit_colony_view();
      return e_input_handled::yes;
    default: //
      return e_input_handled::no;
  }
}

/****************************************************************
** Colony Plane
*****************************************************************/
struct ColonyPlane : public Plane {
  ColonyPlane() = default;
  bool covers_screen() const override { return true; }
  void draw( Texture& tx ) const override {
    draw_colony_view( tx, curr_colony_id );
  }
  e_input_handled input( input::event_t const& event ) override {
    switch( enum_for( event ) ) {
      case input::e_input_event::key_event: {
        auto& val = get_if_or_die<input::key_event_t>( event );
        return handle_key_event( val );
      }
      case input::e_input_event::win_event: {
        auto& val = get_if_or_die<input::win_event_t>( event );
        if( val.type == input::e_win_event_type::resized )
          set_colview_colony( curr_colony_id );
        // Generally we should return no here because this is an
        // event that we want all planes to see.
        return e_input_handled::no;
      }
      default: return e_input_handled::no;
    }
  }

  ColonyId curr_colony_id;
};

ColonyPlane g_colony_plane;

} // namespace

/****************************************************************
** Public API
*****************************************************************/

Plane* colony_plane() { return &g_colony_plane; }

void show_colony_view( ColonyId id ) {
  CHECK( colony_exists( id ) );
  g_colony_plane.curr_colony_id = id;
  set_colview_colony( id );
  push_plane_config( e_plane_config::colony );
}

/****************************************************************
** Menu Handlers
*****************************************************************/

//
MENU_ITEM_HANDLER(
    e_menu_item::colony_view_close, [] { pop_plane_config(); },
    [] { return is_plane_enabled( e_plane::colony ); } )

} // namespace rn
