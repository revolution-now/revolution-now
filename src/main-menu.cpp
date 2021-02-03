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
#include "enum.hpp"
#include "gfx.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "ttf.hpp"

// base-util
#include "base-util/algo.hpp"

// magic enum
#include "magic_enum.hpp"

using namespace std;

namespace rn {

namespace {

e_main_menu_item                   g_curr_item;
waitable_promise<e_main_menu_item> g_promise;

/****************************************************************
** Plane
*****************************************************************/
struct MainMenuPlane : public Plane {
  MainMenuPlane() = default;
  bool covers_screen() const override { return true; }
  void draw( Texture& tx ) const override {
    tile_sprite( tx, e_tile::wood_middle,
                 main_window_logical_rect() );
    auto screen    = main_window_logical_size();
    H    h         = screen.h / 2_sy;
    auto num_items = magic_enum::enum_count<e_main_menu_item>();
    h -= ttf_get_font_info( font::main_menu() ).height *
         SY{ int( num_items ) } / 2_sy;
    for( auto e : magic_enum::enum_values<e_main_menu_item>() ) {
      Color       c       = Color::banana().shaded( 3 );
      auto const& text_tx = render_text(
          font::main_menu(), c, enum_to_display_name( e ) );
      auto w   = screen.w / 2_sx - text_tx.size().w / 2_sx;
      auto dst = text_tx.rect().shifted_by( Delta{ w, h } );
      text_tx.copy_to( tx, /*src=*/nothing,
                       /*dst=*/dst );
      dst = dst.with_border_added( 2 );
      dst.x -= 3_w;
      dst.w += 6_w;
      if( e == g_curr_item )
        render_rect( tx, Color::banana(), dst );
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
                magic_enum::enum_values<e_main_menu_item>(),
                g_curr_item );
            handled = e_input_handled::yes;
            break;
          case ::SDLK_DOWN:
          case ::SDLK_KP_2:
            g_curr_item = util::find_subsequent_and_cycle(
                magic_enum::enum_values<e_main_menu_item>(),
                g_curr_item );
            handled = e_input_handled::yes;
            break;
          case ::SDLK_RETURN:
          case ::SDLK_KP_ENTER:
            // FIXME: this can check-fail if there are two
            // consecutive ENTERs in the input queue.
            CHECK( !g_promise.has_value() );
            g_promise.set_value( g_curr_item );
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
waitable<e_main_menu_item> next_main_menu_item() {
  g_promise = waitable_promise<e_main_menu_item>{};
  return g_promise.get_waitable();
}

} // namespace rn
