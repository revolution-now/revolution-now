/****************************************************************
**main-menu.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Main application menu.
*
*****************************************************************/
#include "main-menu.hpp"

// Revolution Now
#include "compositor.hpp"
#include "enum.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "tiles.hpp"

// render
#include "render/renderer.hpp"

// refl
#include "refl/query-enum.hpp"

// base-util
#include "base-util/algo.hpp"

using namespace std;

namespace rn {

namespace {

e_main_menu_item             g_curr_item;
co::stream<e_main_menu_item> g_selection_stream;

/****************************************************************
** Plane
*****************************************************************/
struct MainMenuPlane : public Plane {
  MainMenuPlane() = default;
  bool covers_screen() const override { return true; }
  void draw( rr::Renderer& renderer ) const override {
    UNWRAP_CHECK(
        normal_area,
        compositor::section( compositor::e_section::normal ) );
    tile_sprite( renderer, e_tile::wood_middle, normal_area );
    H    h         = normal_area.h / 2_sy;
    auto num_items = refl::enum_count<e_main_menu_item>;
    h -= H{ rr::rendered_text_line_size_pixels( "X" ).h } *
         SY{ int( num_items ) } / 2_sy;
    rr::Painter painter = renderer.painter();
    for( auto e : refl::enum_values<e_main_menu_item> ) {
      gfx::pixel c = gfx::pixel::banana().shaded( 3 );
      Delta      text_size =
          Delta::from_gfx( rr::rendered_text_line_size_pixels(
              enum_to_display_name( e ) ) );
      auto w   = normal_area.w / 2_sx - text_size.w / 2_sx;
      auto dst = Rect::from( Coord{}, text_size )
                     .shifted_by( Delta{ w, h } );
      dst = dst.as_if_origin_were( normal_area.upper_left() );
      rr::Typer typer = renderer.typer( dst.upper_left(), c );
      dst             = dst.with_border_added( 2 );
      dst.x -= 3_w;
      dst.w += 6_w;
      if( e == g_curr_item )
        painter.draw_empty_rect(
            dst, rr::Painter::e_border_mode::outside,
            gfx::pixel::banana() );
      h += dst.delta().h;
    }
  }
  e_input_handled input( input::event_t const& event ) override {
    auto handled = e_input_handled::no;
    switch( event.to_enum() ) {
      case input::e_input_event::key_event: {
        auto& val = event.get<input::key_event_t>();
        if( val.change != input::e_key_change::down ) break;
        // It's a key down.
        switch( val.keycode ) {
          case ::SDLK_UP:
          case ::SDLK_KP_8:
            g_curr_item = util::find_previous_and_cycle(
                refl::enum_values<e_main_menu_item>,
                g_curr_item );
            handled = e_input_handled::yes;
            break;
          case ::SDLK_DOWN:
          case ::SDLK_KP_2:
            g_curr_item = util::find_subsequent_and_cycle(
                refl::enum_values<e_main_menu_item>,
                g_curr_item );
            handled = e_input_handled::yes;
            break;
          case ::SDLK_RETURN:
          case ::SDLK_KP_ENTER:
            g_selection_stream.send( g_curr_item );
            handled = e_input_handled::yes;
            break;
          default: break;
        }
        break;
      }
      default: //
        break;
    }
    return handled;
  }
};

MainMenuPlane g_main_menu_plane;

} // namespace

Plane* main_menu_plane() { return &g_main_menu_plane; }

/****************************************************************
** Public API
*****************************************************************/
co::stream<e_main_menu_item>& main_menu_input_stream() {
  return g_selection_stream;
}

} // namespace rn
