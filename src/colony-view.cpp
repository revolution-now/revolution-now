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
#include "compositor.hpp"
#include "cstate.hpp"
#include "menu.hpp"
#include "plane-ctrl.hpp"
#include "plane.hpp"
#include "text.hpp"

// base-util
#include "base-util/variant.hpp"

using namespace std;

namespace rn {

namespace {
//

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

  line( "name: {}", colony.name() );
  line( "id: {}", colony.id() );
  line( "population: {}", colony.population() );
  line( "nation: {}", colony.nation() );
  line( "location: {}", colony.location() );
}

bool handle_key_event( input::key_event_t const& key_event ) {
  if( key_event.change != input::e_key_change::down )
    return false;
  switch( key_event.keycode ) {
    case ::SDLK_ESCAPE: //
      exit_colony_view();
      return true;
    default: //
      return false;
  }
}

auto handle_input = variant_function( event, ->, bool ) {
  case_( input::key_event_t ) {
    return handle_key_event( event );
  }
  default_variant_function( return false );
};

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
    return handle_input( event ) ? e_input_handled::yes
                                 : e_input_handled::no;
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
  push_plane_config( e_plane_config::colony );
}

/****************************************************************
** Menu Handlers
*****************************************************************/

MENU_ITEM_HANDLER(
    e_menu_item::colony_view_close, [] { pop_plane_config(); },
    [] { return is_plane_enabled( e_plane::colony ); } )

} // namespace rn
