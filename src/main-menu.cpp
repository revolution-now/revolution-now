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
#include "gfx.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "ttf.hpp"

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
  void draw( Texture& tx ) const override {
    UNWRAP_CHECK(
        normal_area,
        compositor::section( compositor::e_section::normal ) );
    tile_sprite( tx, e_tile::wood_middle, normal_area );
    H    h         = normal_area.h / 2_sy;
    auto num_items = enum_traits<e_main_menu_item>::count;
    h -= ttf_get_font_info( font::main_menu() ).height *
         SY{ int( num_items ) } / 2_sy;
    for( auto e : enum_traits<e_main_menu_item>::values ) {
      gfx::pixel  c       = gfx::pixel::banana().shaded( 3 );
      auto const& text_tx = render_text(
          font::main_menu(), c, enum_to_display_name( e ) );
      auto w   = normal_area.w / 2_sx - text_tx.size().w / 2_sx;
      auto dst = text_tx.rect().shifted_by( Delta{ w, h } );
      text_tx.copy_to(
          tx, /*src=*/nothing,
          /*dst=*/
          dst.as_if_origin_were( normal_area.upper_left() ) );
      dst = dst.with_border_added( 2 );
      dst.x -= 3_w;
      dst.w += 6_w;
      if( e == g_curr_item )
        render_rect(
            tx, gfx::pixel::banana(),
            dst.as_if_origin_were( normal_area.upper_left() ) );
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
                enum_traits<e_main_menu_item>::values,
                g_curr_item );
            handled = e_input_handled::yes;
            break;
          case ::SDLK_DOWN:
          case ::SDLK_KP_2:
            g_curr_item = util::find_subsequent_and_cycle(
                enum_traits<e_main_menu_item>::values,
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
