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

Opt<e_main_menu_item> g_item_sel;
e_main_menu_type      g_type{ e_main_menu_type::no_game };
e_main_menu_item      g_curr_item{ e_main_menu_item::new_ };
;

bool is_item_enabled( e_main_menu_item item ) {
  switch( item ) {
    case +e_main_menu_item::resume:
      return g_type == e_main_menu_type::in_game;
    case +e_main_menu_item::new_:
      return g_type == e_main_menu_type::no_game;
    case +e_main_menu_item::load:
      return g_type == e_main_menu_type::no_game;
    case +e_main_menu_item::save:
      return g_type == e_main_menu_type::in_game;
    case +e_main_menu_item::leave:
      return g_type == e_main_menu_type::in_game;
    case +e_main_menu_item::quit:
      return g_type == e_main_menu_type::no_game;
  }
  UNREACHABLE_LOCATION;
}

/****************************************************************
** Plane
*****************************************************************/
struct MainMenuPlane : public Plane {
  MainMenuPlane() = default;
  bool enabled() const override { return enabled_; }
  bool covers_screen() const override { return true; }
  void draw( Texture& tx ) const override {
    tile_sprite( tx, g_tile::wood_middle,
                 compositor::section(
                     compositor::e_section::non_menu_bar ) );
    auto screen    = main_window_logical_size();
    H    h         = screen.h / 2_sy;
    auto num_items = values<e_main_menu_item>.size();
    h -= ttf_get_font_info( font::standard() ).height *
         SY{ int( num_items ) } / 2_sy;
    for( auto e : values<e_main_menu_item> ) {
      Color c = is_item_enabled( e ) ? Color::banana()
                                     : Color::black();
      auto const& text_tx =
          render_text( font::standard(), c, e._to_string() );
      auto w   = screen.w / 2_sx - text_tx.size().w / 2_sx;
      auto dst = text_tx.rect().shifted_by( Delta{ w, h } );
      text_tx.copy_to( tx, /*src=*/nullopt,
                       /*dst=*/dst );
      if( e == g_curr_item )
        render_rect( tx, Color::white(), dst );
      h += text_tx.size().h;
    }
  }
  bool input( input::event_t const& event ) override {
    bool handled = false;
    switch_( event ) {
      case_( input::key_event_t ) {
        if( val.change != input::e_key_change::down ) break_;
        // It's a key down.
        switch( val.keycode ) {
          case ::SDLK_UP:
          case ::SDLK_KP_8:
            do {
              g_curr_item = util::find_previous_and_cycle(
                  values<e_main_menu_item>, g_curr_item );
            } while( !is_item_enabled( g_curr_item ) );
            handled = true;
            break;
          case ::SDLK_DOWN:
          case ::SDLK_KP_2:
            do {
              g_curr_item = util::find_subsequent_and_cycle(
                  values<e_main_menu_item>, g_curr_item );
            } while( !is_item_enabled( g_curr_item ) );
            handled = true;
            break;
          case ::SDLK_RETURN:
          case ::SDLK_KP_ENTER:
            CHECK( !g_item_sel.has_value() );
            g_item_sel = g_curr_item;
            handled    = true;
            break;
          case ::SDLK_ESCAPE: //
            if( g_type == e_main_menu_type::in_game )
              g_item_sel = e_main_menu_item::resume;
            handled = true;
            break;
          default: break;
        }
        break_;
      }
      switch_non_exhaustive;
    }
    return handled;
  }
  bool enabled_{ true };
};

MainMenuPlane g_main_menu_plane;

} // namespace

Plane* main_menu_plane() { return &g_main_menu_plane; }

/****************************************************************
** Public API
*****************************************************************/
void show_main_menu_plane() {
  g_main_menu_plane.enabled_ = true;
}
void hide_main_menu_plane() {
  g_main_menu_plane.enabled_ = false;
}

void set_main_menu( e_main_menu_type type ) {
  g_type = type;
  if( !is_item_enabled( g_curr_item ) ) {
    // The currently selected item is no longer enabled, so find
    // the first enabled item.
    for( auto e : values<e_main_menu_item> ) {
      g_curr_item = e;
      if( is_item_enabled( e ) ) break;
    }
  }
}

// When this function returns a value, that value will be reset,
// so another call immediately after will yield no result.
Opt<e_main_menu_item> main_menu_selection() {
  auto res = g_item_sel;
  if( res.has_value() ) g_item_sel = nullopt;
  return res;
}

/****************************************************************
** Testing
*****************************************************************/
void test_main_menu() {
  //
}

} // namespace rn
