/****************************************************************
**menu.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-27.
*
* Description: Menu Bar
*
*****************************************************************/
#include "menu.hpp"

// Revolution Now
#include "adt.hpp"
#include "aliases.hpp"
#include "config-files.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "frame.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "plane.hpp"
#include "ranges.hpp"
#include "screen.hpp"
#include "sdl-util.hpp"
#include "tiles.hpp"
#include "variant.hpp"

// base-util
#include "base-util/algo.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

// C++ standard library
#include <chrono>

using namespace std;

using absl::flat_hash_map;

ADT( rn, MenuState,                   //
     ( menus_hidden ),                //
     ( menus_closed,                  //
       ( Opt<e_menu>, hover ) ),      //
     ( menu_open,                     //
       ( e_menu, menu ),              //
       ( Opt<e_menu_item>, hover ) ), //
     ( item_click,                    //
       ( e_menu_item, item ),         //
       ( TimeType, start ) )          //
);

ADT( rn, MouseOver,
     ( header,                  //
       ( e_menu, menu ) ),      //
     ( item,                    //
       ( e_menu_item, item ) ), //
     ( divider,                 //
       ( e_menu, menu ) ),      //
     ( border,                  //
       ( e_menu, menu ) ),      //
     ( bar )                    //
);

namespace rn {

namespace {

/****************************************************************
** Main Data Structures
*****************************************************************/
struct Menu {
  string name;
  bool   right_side;
  char   shortcut;
};

absl::flat_hash_map<e_menu, Menu> g_menus{
    {e_menu::game, {"Game", false, 'G'}},
    {e_menu::view, {"View", false, 'V'}},
    {e_menu::orders, {"Orders", false, 'O'}},
    {e_menu::advisors, {"Advisors", false, 'A'}},
    {e_menu::debug, {"Debug", true, 'D'}},
    {e_menu::pedia, {"Revolopedia", true, 'R'}}};

struct MenuDivider {};

struct MenuCallbacks {
  function<void( void )> on_click;
  function<bool( void )> enabled;
};

// menu.cpp
struct MenuClickable {
  e_menu_item   item;
  string        name;
  MenuCallbacks callbacks;
};

using MenuItem = variant<MenuDivider, MenuClickable>;

flat_hash_map<e_menu_item, MenuClickable*> g_menu_items;
flat_hash_map<e_menu_item, e_menu>         g_item_to_menu;
flat_hash_map<e_menu, Vec<e_menu_item>>    g_items_from_menu;

#define ITEM( item, name )      \
  MenuClickable {               \
    e_menu_item::item, name, {} \
  }

#define DIVIDER \
  MenuDivider {}

/****************************************************************
** The Menus
*****************************************************************/
absl::flat_hash_map<e_menu, Vec<MenuItem>> g_menu_def{
    {e_menu::game,
     {
         ITEM( about, "About this Game" ),    //
         /***********/ DIVIDER, /***********/ //
         ITEM( revolution, "REVOLUTION!" ),   //
         /***********/ DIVIDER, /***********/ //
         ITEM( retire, "Retire" ),            //
         ITEM( exit, "Exit to \"DOS\"" )      //
     }},
    {e_menu::view,
     {
         ITEM( zoom_in, "Zoom In" ),          //
         ITEM( zoom_out, "Zoom Out" ),        //
         /***********/ DIVIDER, /***********/ //
         ITEM( restore_zoom, "Zoom Default" ) //
     }},
    {e_menu::orders,
     {
         ITEM( sentry, "Sentry" ),  //
         ITEM( fortify, "Fortify" ) //
     }},
    {e_menu::advisors,
     {
         ITEM( military_advisor, "Military Advisor" ),   //
         ITEM( economics_advisor, "Economics Advisor" ), //
         ITEM( european_advisor, "Europian Advisor" )    //
     }},
    {e_menu::debug,
     {
         ITEM( toggle_console, "Toggle Console" ) //
     }},
    {e_menu::pedia,
     {
         ITEM( units_help, "Units" ),                     //
         ITEM( terrain_help, "Terrain" ),                 //
         ITEM( founding_father_help, "Founding Fathers" ) //
     }}};

/****************************************************************
** Menu State
*****************************************************************/
namespace click_anim {

// TODO: make an animation framework that can manage the states
// of an animation along with durations in Frames.

// For the MacOS single-blink-and-fade style.
auto constexpr half_period      = Frames{6};
auto constexpr post_off_time    = Frames{0};
auto constexpr num_half_periods = int( 2 );
bool constexpr start_on         = false;
auto constexpr fade_time        = Frames{22};

// For example, the click could be like this:
//
//   click_anim_start_on=true, num_half_periods=4
//   ----------------------------------------------------
//   |  on  |  off |  on  |  off | off-time | fade-time |
//   ----------------------------------------------------
//
// where each box is a "half period."

auto period = half_period * 2;
auto total_duration =
    half_period * num_half_periods + post_off_time + fade_time;
} // namespace click_anim

MenuState_t g_menu_state{MenuState::menus_closed{}};

void log_menu_state() {
  logger->debug( "g_menu_state: {}", g_menu_state );
}

/****************************************************************
** Querying State
*****************************************************************/
bool is_menu_open( e_menu menu ) {
  auto matcher = scelta::match(
      []( MenuState::menus_hidden ) { return false; },
      []( MenuState::menus_closed ) { return false; },
      [&]( MenuState::item_click click ) {
        CHECK( g_item_to_menu.contains( click.item ) );
        return g_item_to_menu[click.item] == menu;
      },
      [&]( MenuState::menu_open o ) { return o.menu == menu; } );
  return matcher( g_menu_state );
}

Opt<e_menu> opened_menu() {
  if_v( g_menu_state, MenuState::menu_open, val ) {
    return val->menu;
  }
  return {};
}

bool is_menu_item_enabled( e_menu_item item ) {
  event_counts()["menu.cpp imie"].tick();
  CHECK( g_menu_items.contains( item ) );
  return g_menu_items[item]->callbacks.enabled();
}

bool is_menu_visible( e_menu menu ) {
  event_counts()["menu.cpp imv"].tick();
  auto enabled_items = g_items_from_menu[menu] |
                       rv::filter( is_menu_item_enabled );
  return enabled_items.begin() != enabled_items.end();
}

auto visible_menus() {
  return values<e_menu> | rv::filter( is_menu_visible );
}

bool have_some_visible_menus() {
  auto rng = visible_menus();
  return std::distance( rng.begin(), rng.end() ) > 0;
}

/****************************************************************
** Colors
*****************************************************************/
namespace color::item::background {
auto active() {
  auto color = config_palette.yellow.sat1.lum11;
  return color;
}
} // namespace color::item::background
namespace color::item::foreground {
auto const& active   = config_palette.orange.sat0.lum4;
auto const& inactive = config_palette.orange.sat1.lum6;
auto const& disabled = config_palette.grey.n68;
} // namespace color::item::foreground

namespace color::menu::background {
auto active() {
  auto color = color::item::background::active();
  return color;
}
auto hover() {
  auto color = config_palette.orange.sat0.lum3;
  color      = color.highlighted( 2 );
  return color;
}
} // namespace color::menu::background
namespace color::menu::foreground {
auto const& active   = config_palette.orange.sat0.lum2;
auto const& inactive = config_palette.orange.sat1.lum11;
auto const& disabled = config_palette.grey.n68;
} // namespace color::menu::foreground

/****************************************************************
** Cached Textures
*****************************************************************/
struct ItemTextures {
  Texture normal;
  Texture highlighted;
  Texture disabled;
  // This is the max width of the above textures, just for the
  // sake of having it precomputed.
  W width{0};
};

flat_hash_map<e_menu_item, ItemTextures> g_menu_item_rendered;

struct MenuTextures {
  // Each menu may have a different width depending on the sizes
  // of the elements inside of it, so we will render different
  // sized dividers for each menu. All dividers in a given menu
  // will be the same size though.
  Texture      divider;
  Texture      item_background_highlight;
  ItemTextures name;
  Texture      menu_body;
  Texture      menu_body_shadow;
  Texture      menu_background_highlight;
  Texture      menu_background_hover;
  W            header_width{0};
};

absl::flat_hash_map<e_menu, MenuTextures> g_menu_rendered;

Texture menu_bar_tx;

// Must be called after all textures are rendered. It will it-
// erate through all the rendered text textures (both menu
// headers and menu items) and will compute the maximum height of
// all of them. This height will then be used as the height for
// all text elements in the menus for simplicity.
//
// This value is cached because a) it requires querying textures
// and b) its value is not expected to ever change.
H const& max_text_height() {
  static H max_height = [] {
    H res{0};
    for( auto menu : values<e_menu> ) {
      CHECK( g_menu_rendered.contains( menu ) );
      auto const& textures = g_menu_rendered[menu];
      CHECK( textures.name.normal );
      res = std::max( res, textures.name.normal.size().h );
      for( auto item : g_items_from_menu[menu] ) {
        CHECK( g_menu_item_rendered.contains( item ) );
        auto const& textures = g_menu_item_rendered[item];
        // These are probably all the same size, but just in case
        // they aren't. The 2_sx is because we have padding on
        // each side of the item.
        res = std::max( res, textures.normal.size().h );
        res = std::max( res, textures.highlighted.size().h );
        res = std::max( res, textures.disabled.size().h );
      }
      return res;
    }
    CHECK( res > 0_h );
    return res;
  }();
  return max_height;
}

/****************************************************************
** Geometry
*****************************************************************/
H menu_bar_height() { return 16_h; }

Delta menu_header_delta( e_menu menu ) {
  CHECK( g_menu_rendered.contains( menu ) );
  return Delta{g_menu_rendered[menu].header_width,
               max_text_height()};
}

// These cannot be precalculated because menus might be hidden.
X menu_header_x_pos( e_menu target ) {
  CHECK( g_menus.contains( target ) );
  CHECK( is_menu_visible( target ) );
  auto const& desc = g_menus[target];
  W           width_delta{0};
  if( desc.right_side ) {
    width_delta = ranges::accumulate(
        visible_menus()                                      //
            | rv::reverse                                    //
            | rv::remove_if( L( !g_menus[_].right_side ) )   //
            | take_while_inclusive( LC( _ != target ) )      //
            | rv::transform( L( menu_header_delta( _ ).w ) ) //
            | rv::intersperse( config_ui.menus.spacing ),
        0_w );
  } else {
    width_delta = ranges::accumulate(
        visible_menus()                                   //
            | rv::remove_if( L( g_menus[_].right_side ) ) //
            | rv::take_while( LC( _ != target ) )         //
            | rv::transform( L( menu_header_delta( _ ).w +
                                config_ui.menus.spacing ) ),
        0_w );
  }
  width_delta += config_ui.menus.first_menu_start;
  CHECK( width_delta != 0_w );
  return 0_x + ( !desc.right_side
                     ? width_delta
                     : screen_logical_size().w - width_delta );
}

// Rectangle around a menu header.
Rect menu_header_rect( e_menu menu ) {
  CHECK( g_menu_rendered.contains( menu ) );
  return Rect::from( Coord{0_y, menu_header_x_pos( menu )},
                     menu_header_delta( menu ) );
}

// The long, thin rectangle around the top menu bar. This extends
// from the left side of the screen to the right, and includes
// the menu headers. but does not include the space that would be
// occupied by open menu bodies.
Rect menu_bar_rect() {
  auto delta_screen = screen_logical_size();
  auto height       = menu_bar_height();
  return Rect::from( Coord{}, Delta{delta_screen.w, height} );
}

// This is the width of the menu body not including the borders,
// which themselves occupy part of a tile.
W menu_body_width_inner( e_menu menu ) {
  W res{0};
  for( auto const& item : g_items_from_menu[menu] ) {
    CHECK( g_menu_item_rendered.contains( item ) );
    res = std::max( res, g_menu_item_rendered[item].width );
  }
  // At this point, res holds the width of the largest rendered
  // text texture in this menu.  Now add padding on each side:
  res += config_ui.menus.padding * 2_sx;
  res = clamp( res, config_ui.menus.body_min_width, 1000000_w );
  // round up to nearest multiple of 8, since that is the menu
  // tile width.
  if( res % 8_sx != 0_w ) res += ( 8_w - ( res % 8_sx ) );
  // Sanity check
  CHECK( res > 0 && res < 2000 );
  return res;
}

H divider_height() { return max_text_height() / 2; }

// This is the width of the menu body not including the borders,
// which themselves occupy part of a tile.
H menu_body_height_inner( e_menu menu ) {
  H    h{0};
  auto add_height = scelta::match(
      [&]( MenuDivider ) { h += divider_height(); },
      [&]( MenuClickable ) { h += max_text_height(); } );
  CHECK( g_menu_def.contains( menu ) );
  for( auto const& item : g_menu_def[menu] ) add_height( item );
  // round up to nearest multiple of 8, since that is the menu
  // tile width.
  if( h % 8_sy != 0_h ) h += ( 8_h - ( h % 8_sy ) );
  return h;
}

Delta menu_body_delta_inner( e_menu menu ) {
  return {menu_body_width_inner( menu ),
          menu_body_height_inner( menu )};
}

Delta menu_body_delta( e_menu menu ) {
  return Delta{8_w, 8_h} + Delta{8_w, 8_h} +
         menu_body_delta_inner( menu );
}

Delta menu_item_delta( e_menu menu ) {
  return Delta{menu_body_width_inner( menu ), max_text_height()};
}

Delta divider_delta( e_menu menu ) {
  return Delta{divider_height(), menu_body_width_inner( menu )};
}

Rect menu_body_rect_inner( e_menu menu ) {
  CHECK( g_menus.contains( menu ) );
  if( g_menus[menu].right_side ) {
    Coord pos{Y{0} + menu_bar_height() + 8_h,
              menu_header_x_pos( menu ) +
                  menu_header_delta( menu ).w -
                  menu_body_delta_inner( menu ).w};
    return Rect::from( pos, menu_body_delta_inner( menu ) );
  } else {
    Coord pos{Y{0} + menu_bar_height() + 8_h,
              menu_header_x_pos( menu )};
    return Rect::from( pos, menu_body_delta_inner( menu ) );
  }
}

Rect menu_body_rect( e_menu menu ) {
  CHECK( g_menus.contains( menu ) );
  if( g_menus[menu].right_side ) {
    Coord pos{Y{0} + menu_bar_height(),
              menu_header_x_pos( menu ) +
                  menu_header_delta( menu ).w + 8_w -
                  menu_body_delta( menu ).w};
    return Rect::from( pos, menu_body_delta( menu ) );
  } else {
    Coord pos{Y{0} + menu_bar_height(),
              menu_header_x_pos( menu ) - 8_w};
    return Rect::from( pos, menu_body_delta( menu ) );
  }
}

// This includes (roughly) the space overwhich an open menu
// occupies pixels (i.e., is not transparent). This is used to
// decide if the user has clicked on or off of an open menu.
Rect menu_body_clickable_area( e_menu menu ) {
  auto res = menu_body_rect( menu );
  res.x += 8_w / 2_sx;
  res.w -= 8_w / 2_sx * 2_sx;
  res.h -= 8_h / 2_sy;
  return res;
}

// `h` is the vertical position from the top of the menu body.
Opt<e_menu_item> cursor_to_item( e_menu menu, H h ) {
  CHECK( g_menu_def.contains( menu ) );
  H pos{0};
  for( auto const& item : g_menu_def[menu] ) {
    auto advance = scelta::match(
        [&]( MenuDivider ) { pos += divider_height(); },
        [&]( MenuClickable const& ) {
          pos += max_text_height();
        } );
    advance( item );
    if( pos > h ) {
      return scelta::match(
          [&]( MenuDivider ) { return Opt<e_menu_item>{}; },
          [&]( MenuClickable const& clickable ) {
            return Opt<e_menu_item>( clickable.item );
          } //
          )( item );
    }
  }
  return {};
}

// `cursor` is the screen position of the mouse cursor.
Opt<e_menu_item> cursor_to_item( e_menu menu, Coord cursor ) {
  if( !cursor.is_inside( menu_body_rect_inner( menu ) ) )
    return {};
  return cursor_to_item(
      menu, cursor.y - menu_body_rect_inner( menu ).top_edge() );
}

/****************************************************************
** Rendering Implmementation
*****************************************************************/
// For either a menu header or item.
ItemTextures render_menu_element( string const& text,
                                  optional<char> /*unused*/,
                                  Color inactive_color,
                                  Color active_color,
                                  Color disabled_color ) {
  auto inactive = render_text_line_shadow(
      fonts::standard, inactive_color, text );
  auto active   = render_text_line_shadow( fonts::standard,
                                         active_color, text );
  auto disabled = render_text_line_shadow(
      fonts::standard, disabled_color, text );
  // Need to do this first before moving.
  auto width = std::max(
      {inactive.size().w, active.size().w, disabled.size().w} );
  auto res =
      ItemTextures{std::move( inactive ), std::move( active ),
                   std::move( disabled ), width};
  // Sanity check
  CHECK( res.width > 0 && res.width < screen_logical_size().w );
  return res;
}

// For either a menu header or item.
ItemTextures render_menu_item_element(
    string const& text, optional<char> /*unused*/ ) {
  return render_menu_element(
      text, nullopt, //
      color::item::foreground::inactive,
      color::item::foreground::active,
      color::item::foreground::disabled );
}

// For either a menu header or item.
ItemTextures render_menu_header_element(
    string const& text, optional<char> /*unused*/ ) {
  return render_menu_element(
      text, nullopt, //
      color::menu::foreground::inactive,
      color::menu::foreground::active,
      color::menu::foreground::disabled );
}

Texture render_divider( e_menu menu ) {
  Delta   delta = divider_delta( menu );
  Texture res   = create_texture_transparent( delta );
  // A divider is never highlighted.
  Color color_fore = color::item::foreground::disabled;
  Color color_back = color_fore.shaded( 4 );
  render_line( res, color_fore,
               Coord{} + delta.h / 2 - 1_h + 2_w,
               {delta.w - 5_w, 0_h} );
  render_line( res, color_back,
               Coord{} + delta.h / 2 + 2_w + 1_w - 0_h,
               {delta.w - 5_w, 0_h} );
  return res;
}

Texture create_menu_body_texture( e_menu menu ) {
  return create_texture_transparent( menu_body_delta( menu ) );
}

Texture render_item_background( e_menu menu, bool active ) {
  CHECK( active );
  return create_texture( menu_item_delta( menu ),
                         color::item::background::active() );
}

Texture render_menu_header_background( e_menu menu, bool active,
                                       bool hover ) {
  CHECK( active || hover );
  // FIXME
  CHECK( !( active && hover ) );
  auto color = active ? color::menu::background::active()
                      : color::menu::background::hover();
  return create_texture( menu_header_delta( menu ), color );
}

Texture const& render_open_menu( e_menu           menu,
                                 Opt<e_menu_item> subject,
                                 bool             clicking ) {
  CHECK( g_menu_rendered.contains( menu ) );
  if( subject.has_value() ) {
    CHECK( g_item_to_menu[*subject] == menu );
  }
  auto& textures = g_menu_rendered[menu];
  auto& dst      = textures.menu_body;
  Coord pos{8_x, 8_y};

  clear_texture_transparent( dst );

  render_rect_of_sprites_with_border(
      dst,                            //
      Coord{},                        //
      dst.size() / Scale{8_sx, 8_sy}, //
      g_tile::menu_body,              //
      g_tile::menu_top,               //
      g_tile::menu_bottom,            //
      g_tile::menu_left,              //
      g_tile::menu_right,             //
      g_tile::menu_top_left,          //
      g_tile::menu_top_right,         //
      g_tile::menu_bottom_left,       //
      g_tile::menu_bottom_right       //
  );

  auto render = scelta::match(
      [&]( MenuDivider ) {
        copy_texture( textures.divider, dst, pos );
        pos += divider_height();
      },
      [&]( MenuClickable const& clickable ) {
        auto const& desc = g_menu_items[clickable.item];
        auto const& rendered =
            g_menu_item_rendered[clickable.item];
        Texture const* foreground{nullptr};
        Texture const* background{nullptr};
        if( clicking && clickable.item == subject ) {
          /**********************************************
          ** Click Animation
          ***********************************************/
          using namespace std::chrono;
          using namespace std::literals::chrono_literals;
          auto now = system_clock::now();
          CHECK( util::holds<MenuState::item_click>(
              g_menu_state ) );
          auto start =
              std::get<MenuState::item_click>( g_menu_state )
                  .start;
          auto elapsed = now - start;
          using namespace click_anim;
          if( elapsed >= total_duration - fade_time ) {
            // We're in the fade, in which case we should be
            // highlighted, because it seems to make a better
            // UX when the selected item is highlighted as
            // the fading happens.
            foreground = &rendered.highlighted;
            background = &textures.item_background_highlight;
          } else if( elapsed >= total_duration - fade_time -
                                    post_off_time ) {
            // We're in a period between the blinking and
            // fading when the highlight is off, although
            // this period may have zero length.
            foreground = &rendered.normal;

          } else if( ( elapsed % period > half_period ) ^
                     start_on ) {
            // Blink on
            foreground = &rendered.highlighted;
            background = &textures.item_background_highlight;
          } else {
            // Blink off
            foreground = &rendered.normal;
          }
        } else {
          foreground = !desc->callbacks.enabled()
                           ? &rendered.disabled
                           : ( clickable.item == subject )
                                 ? &rendered.highlighted
                                 : &rendered.normal;
          if( clickable.item == subject )
            background = &textures.item_background_highlight;
        }
        if( background )
          copy_texture_alpha( *background, dst, pos, 128 );
        CHECK( foreground );
        copy_texture( *foreground, dst,
                      pos + config_ui.menus.padding );
        pos += max_text_height();
      } );

  for( auto const& item : g_menu_def[menu] ) render( item );
  return dst;
}

void render_menu_bar() {
  CHECK( menu_bar_tx );
  auto bar_tx_size = menu_bar_tx.size();
  auto lower_right = ( Coord{} + bar_tx_size )
                         .rounded_up_to_multiple( Scale{8} );
  auto tiles_rect =
      Rect::from( Coord{}, lower_right ) / Scale{8};
  for( auto coord : tiles_rect ) {
    if( coord.y == 0_y )
      render_sprite_grid( menu_bar_tx, g_tile::menu_bar_top,
                          coord, 0, 0 );
    else
      render_sprite_grid( menu_bar_tx, g_tile::menu_bar_bottom,
                          coord, 0, 0 );
  }
  // Center the text vertically in the menu bar.
  auto offset = 0_y + ( ( 16_h - max_text_height() ) / 2_sy );

  for( auto menu : visible_menus() ) {
    CHECK( g_menu_rendered.contains( menu ) );
    auto const& textures = g_menu_rendered[menu];
    using Txs = Opt<pair<Texture const*, Texture const*>>;
    // Given `menu`, this matcher visits the global menu state
    // and returns a foreground/background texture pair for that
    // menu.
    auto matcher = scelta::match<Txs>(
        []( MenuState::menus_hidden ) { return Txs{}; },
        [&]( MenuState::menus_closed closed ) {
          if( menu == closed.hover )
            return Txs{pair{&textures.name.normal,
                            &textures.menu_background_hover}};
          return Txs{pair{&textures.name.normal, nullptr}};
        } )( //
        [&]( auto self, MenuState::item_click const& ic ) {
          // Just forward this to the MenuState::menu_open.
          CHECK( g_item_to_menu.contains( ic.item ) );
          return self( MenuState_t{MenuState::menu_open{
              g_item_to_menu[ic.item], /*hover=*/{}}} );
        },
        [&]( auto self, MenuState::menu_open const& o ) {
          if( o.menu == menu ) {
            return Txs{
                pair{&textures.name.highlighted,
                     &textures.menu_background_highlight}};
          } else
            return self(
                MenuState_t{MenuState::menus_closed{}} );
        } );
    if( auto p = matcher( g_menu_state ); p.has_value() ) {
      auto pos = menu_header_x_pos( menu );
      if( ( *p ).second )
        copy_texture( *( ( *p ).second ), menu_bar_tx,
                      {offset, pos} );
      CHECK( ( *p ).first );
      copy_texture( *( ( *p ).first ), menu_bar_tx,
                    {offset, pos + config_ui.menus.padding} );
    }
  }
}

void display_menu_bar_tx( Texture const& tx ) {
  render_menu_bar();
  CHECK( tx.size().w == menu_bar_tx.size().w );
  copy_texture( menu_bar_tx, tx, Coord{} );
}

/****************************************************************
** Input Implementation
*****************************************************************/
Opt<MouseOver_t> click_target( Coord screen_coord ) {
  event_counts()["click target()"].tick();
  using res_t  = Opt<MouseOver_t>;
  auto matcher = scelta::match<res_t>(
      []( MenuState::menus_hidden ) { return res_t{}; },
      [&]( MenuState::menus_closed ) {
        for( auto menu : visible_menus() )
          if( screen_coord.is_inside(
                  menu_header_rect( menu ) ) )
            return res_t{MouseOver::header{menu}};
        if( screen_coord.is_inside( menu_bar_rect() ) )
          return res_t{MouseOver::bar{}};
        return res_t{};
      } )( //
      [&]( auto self, MenuState::item_click const& ic ) {
        // Just forward this to the MenuState::menu_open.
        CHECK( g_item_to_menu.contains( ic.item ) );
        return self( MenuState_t{MenuState::menu_open{
            g_item_to_menu[ic.item], /*hover=*/{}}} );
      },
      [&]( auto self, MenuState::menu_open const& o ) {
        auto closed =
            self( MenuState_t{MenuState::menus_closed{}} );
        if( closed ) return res_t{closed};
        if( !screen_coord.is_inside(
                menu_body_clickable_area( o.menu ) ) )
          return res_t{};
        // The cursor is over a non-transparent part of the open
        // menu.
        if( !screen_coord.is_inside(
                menu_body_rect_inner( o.menu ) ) )
          return res_t{MouseOver::border{o.menu}};
        // The cursor is over the inner menu body, so at this
        // point its either over an item or a divider.
        auto maybe_item = cursor_to_item( o.menu, screen_coord );
        if( !maybe_item.has_value() )
          return res_t{MouseOver::divider{o.menu}};
        // Finally, we are over an item.
        return res_t{MouseOver::item{*maybe_item}};
      } );
  return matcher( g_menu_state );
}

} // namespace

/****************************************************************
** Top-level Render Method
*****************************************************************/
void render_menus( Texture const& tx ) {
  display_menu_bar_tx( tx );
  auto maybe_render_open_menu = scelta::match(
      []( MenuState::menus_hidden ) {},  //
      [&]( MenuState::menus_closed ) {}, //
      [&]( MenuState::item_click const& ic ) {
        auto        menu = g_item_to_menu[ic.item];
        auto const& shadow =
            g_menu_rendered[menu].menu_body_shadow;
        CHECK( shadow );
        // Just forward this to the MenuState::menu_open.
        CHECK( g_item_to_menu.contains( ic.item ) );
        auto const& open_tx =
            render_open_menu( menu, ic.item, /*clicking=*/true );
        Coord   pos   = menu_body_rect( menu ).upper_left();
        uint8_t alpha = 255;
        auto    now   = chrono::system_clock::now();
        auto    start_fade =
            ic.start + ( click_anim::total_duration -
                         click_anim::fade_time );
        auto end = ic.start + click_anim::total_duration;
        if( now >= start_fade ) {
          if( now < end ) {
            double percent_completion =
                double( chrono::duration_cast<
                            std::chrono::milliseconds>(
                            now - start_fade )
                            .count() ) /
                double( chrono::duration_cast<
                            std::chrono::milliseconds>(
                            click_anim::fade_time )
                            .count() );
            CHECK( percent_completion >= 0.0 &&
                       percent_completion <= 1.0,
                   "percent_completion: {}",
                   percent_completion );
            uint8_t alpha_shadow =
                64 - uint8_t( 64.0 * percent_completion );
            copy_texture_alpha( shadow, tx,
                                pos + Delta{5_w, 5_h},
                                alpha_shadow );
            alpha = 255 - uint8_t( 255.0 * percent_completion );
            copy_texture_alpha( open_tx, tx, pos, alpha );
          } else if( now >= end ) {
            alpha = 0;
            copy_texture_alpha( open_tx, tx, pos, alpha );
          }
        } else {
          copy_texture_alpha( shadow, tx, pos + Delta{5_w, 5_h},
                              64 );
          copy_texture( open_tx, tx, pos );
        }
      },
      [&]( MenuState::menu_open const& o ) {
        Coord pos = menu_body_rect( o.menu ).upper_left();

        auto const& shadow =
            g_menu_rendered[o.menu].menu_body_shadow;
        CHECK( shadow );
        copy_texture_alpha( shadow, tx, pos + Delta{5_w, 5_h},
                            64 );
        auto const& open_tx = render_open_menu(
            o.menu, o.hover, /*clicking=*/false );
        pos = menu_body_rect( o.menu ).upper_left();
        copy_texture( open_tx, tx, pos );
      } );
  maybe_render_open_menu( g_menu_state );
}

/****************************************************************
** Registration
*****************************************************************/
auto& on_click_handlers() {
  static absl::flat_hash_map<e_menu_item,
                             std::function<void( void )>>
      m;
  return m;
}

auto& is_enabled_handlers() {
  static absl::flat_hash_map<e_menu_item,
                             std::function<bool( void )>>
      m;
  return m;
}

void register_menu_item_handler(
    e_menu_item                        item,
    std::function<void( void )> const& on_click,
    std::function<bool( void )> const& is_enabled ) {
  CHECK( on_click );
  CHECK( is_enabled );
  auto& on_clicks   = on_click_handlers();
  auto& is_enableds = is_enabled_handlers();
  CHECK( !on_clicks.contains( item ) );
  CHECK( !is_enableds.contains( item ) );
  on_clicks[item]   = on_click;
  is_enableds[item] = is_enabled;
}

/****************************************************************
** Initialization
*****************************************************************/
void init_menus() {
  // Check that all menus have descriptors.
  for( auto menu : values<e_menu> ) {
    CHECK( g_menus.contains( menu ) );
    CHECK( g_menu_def.contains( menu ) );
  }

  // Populate the e_menu_item maps and verify no duplicates.
  for( auto& [menu, items] : g_menu_def ) {
    for( auto& item_desc : items ) {
      if( util::holds<MenuDivider>( item_desc ) ) continue;
      CHECK( util::holds<MenuClickable>( item_desc ) );
      auto& clickable = get<MenuClickable>( item_desc );
      g_items_from_menu[menu].push_back( clickable.item );
      CHECK( !g_menu_items.contains( clickable.item ) );
      g_menu_items[clickable.item] = &clickable;
      CHECK( !g_item_to_menu.contains( clickable.item ) );
      g_item_to_menu[clickable.item] = menu;
    }
  }

  // Check that all menus have at least one item.
  for( auto menu : values<e_menu> ) {
    CHECK( g_items_from_menu[menu].size() > 0 );
  }

  // Check that all e_menu_items are in a menu.
  for( auto item : values<e_menu_item> ) {
    CHECK( g_menu_items.contains( item ) );
    CHECK( g_menu_items[item] != nullptr );
    CHECK( g_item_to_menu.contains( item ) );
  }

  // Populate Callbacks
  for( auto const& [item, func] : on_click_handlers() ) {
    CHECK( !g_menu_items[item]->callbacks.on_click );
    g_menu_items[item]->callbacks.on_click = func;
  }
  for( auto const& [item, func] : is_enabled_handlers() ) {
    CHECK( !g_menu_items[item]->callbacks.enabled );
    g_menu_items[item]->callbacks.enabled = func;
  }

  // Check that all e_menu_items have registered handlers.
  for( auto item : values<e_menu_item> ) {
    auto const& desc = *g_menu_items[item];
    CHECK( item == desc.item );
    CHECK( desc.name.size() > 0 );
    CHECK( desc.callbacks.on_click && desc.callbacks.enabled,
           "the menu item `{}` does not have callback handlers "
           "registered",
           item );
  }

  // Render Menu and Menu-item names. These have to be done first
  // because other things need to be calculated from the sizes of
  // the rendered text.
  for( auto menu_item : values<e_menu_item> )
    g_menu_item_rendered[menu_item] = render_menu_item_element(
        g_menu_items[menu_item]->name, nullopt );
  for( auto menu : values<e_menu> ) {
    g_menu_rendered[menu]      = {};
    g_menu_rendered[menu].name = render_menu_header_element(
        g_menus[menu].name, g_menus[menu].shortcut );
  }

  for( auto menu : values<e_menu> ) {
    // The order in which these are done matters, unfortunately,
    // because some of the functions below rely on results from
    // the previous ones.
    g_menu_rendered[menu].header_width =
        g_menu_rendered[menu].name.normal.size().w +
        config_ui.menus.padding * 2_sx;
    g_menu_rendered[menu].item_background_highlight =
        render_item_background( menu, /*hightlight=*/true );
    g_menu_rendered[menu].menu_body =
        create_menu_body_texture( menu );
    g_menu_rendered[menu].menu_background_highlight =
        render_menu_header_background( menu,
                                       /*highlight=*/true,
                                       /*hover=*/false );
    g_menu_rendered[menu].menu_background_hover =
        render_menu_header_background( menu,
                                       /*highlight=*/false,
                                       /*hover=*/true );

    g_menu_rendered[menu].divider = render_divider( menu );

    // This will render to the g_menu_rendered[menu].menu_body
    // texture an open menu with nothing selected. This is
    // necessary so that we can then convert it to a shadow. We
    // only need to render the shadow once, because all that
    // matters about it is the shape, which will not change even
    // as the open menu body changes (i.e., items are
    // highlighted).
    auto const& to_be_shadowed =
        render_open_menu( menu, nullopt, false );
    g_menu_rendered[menu].menu_body_shadow =
        create_shadow_texture( to_be_shadowed );
  }

  menu_bar_tx = create_texture( menu_bar_rect().delta() );
}

void cleanup_menus() {
  // This should free all the textures representing the menu
  // items.
  g_menu_rendered.clear();
  g_menu_item_rendered.clear();
  menu_bar_tx.free();
}

REGISTER_INIT_ROUTINE( menus, init_menus, cleanup_menus );

/****************************************************************
** Handlers (temporary)
*****************************************************************/
function<void( void )> empty_handler = [] {};
function<bool( void )> enabled_true  = [] { return true; };
function<bool( void )> enabled_false = [] { return false; };
function<bool( void )> quit_handler  = [] {
  throw exception_exit{};
  return false;
};

MENU_ITEM_HANDLER( exit, quit_handler, enabled_true );

/****************************************************************
** The Menu Plane
*****************************************************************/
struct MenuPlane : public Plane {
  MenuPlane() = default;
  bool enabled() const override { return true; }
  bool covers_screen() const override { return false; }
  Plane::e_accept_drag can_drag( input::e_mouse_button button,
                                 Coord origin ) override {
    if( !click_target( origin ).has_value() )
      return Plane::e_accept_drag::no;
    return ( button == input::e_mouse_button::l )
               ? Plane::e_accept_drag::yes
               : Plane::e_accept_drag::swallow;
  }
  // We handle dragging but do not really treat it as a drag; in-
  // stead  we  just receive the dragging events and convert them
  // to the appropriate mouse motion/button event and  feed  that
  // event back through the input() method in order  to  get  the
  // behavior that we want but avoid code duplication.
  void on_drag( input::e_mouse_button /*unused*/,
                Coord /*unused*/, Coord prev,
                Coord current ) override {
    // Convert to mouse motion event.
    input::mouse_move_event_t event{{current}, prev};
    this->input( event );
  }
  void on_drag_finished( input::e_mouse_button button,
                         Coord start, Coord end ) override {
    // If the drag started and ended on the same menu element
    // then do nothing, just leave the menu open. This will pre-
    // vent the interface from acting flaky when e.g. the user
    // attempts to click on a menu header but accidentally per-
    // forms a tiny drag and then the menu quickly opens and
    // closes, when it should actually have just stayed open.
    auto click_start = click_target( start );
    if( click_target( start ) == click_target( end ) &&
        click_start.has_value() &&
        util::holds<MouseOver::header>( *click_start ) )
      return;
    if( button == input::e_mouse_button::l ) {
      // Convert to mouse button event.
      auto buttons = input::e_mouse_button_event::left_up;
      input::mouse_button_event_t event{{end}, buttons};
      this->input( event );
    }
  }
  void draw( Texture const& tx ) const override {
    clear_texture_transparent( tx );
    render_menus( tx );
    // TODO: put this code in a dedicated plane `tick` callback.
    if_v( g_menu_state, MenuState::item_click, val ) {
      auto item  = val->item;
      auto start = val->start;
      if( chrono::system_clock::now() - start >=
          click_anim::total_duration ) {
        g_menu_items[item]->callbacks.on_click();
        g_menu_state = MenuState::menus_closed{/*hover=*/{}};
      }
    }
  }
  bool input( input::event_t const& event ) override {
    auto matcher = scelta::match(
        []( input::unknown_event_t ) { return false; },
        []( input::quit_event_t ) { return false; },
        [&]( input::key_event_t const& key_event ) {
          auto menu = opened_menu();
          if( !menu ) return false;
          if( key_event.change == input::e_key_change::down ) {
            switch( key_event.keycode ) {
              case ::SDLK_RETURN:
              case ::SDLK_KP_ENTER:
                if_v( g_menu_state, MenuState::menu_open, val ) {
                  if( val->hover.has_value() ) {
                    click_menu_item( val->hover.value() );
                    log_menu_state();
                  }
                }
                return true;
              case ::SDLK_ESCAPE:
                g_menu_state = MenuState::menus_closed{{}};
                log_menu_state();
                return true;
              case ::SDLK_KP_4:
              case ::SDLK_LEFT: {
                CHECK( have_some_visible_menus() );
                do {
                  menu = util::find_previous_and_cycle(
                      values<e_menu>, *menu );
                } while( !is_menu_visible( *menu ) );
                CHECK( menu );
                g_menu_state =
                    MenuState::menu_open{*menu, /*hover=*/{}};
                log_menu_state();
                return true;
              }
              case ::SDLK_KP_6:
              case ::SDLK_RIGHT: {
                CHECK( have_some_visible_menus() );
                do {
                  menu = util::find_subsequent_and_cycle(
                      values<e_menu>, *menu );
                } while( !is_menu_visible( *menu ) );
                CHECK( menu );
                g_menu_state =
                    MenuState::menu_open{*menu, /*hover=*/{}};
                log_menu_state();
                return true;
              }
              case ::SDLK_KP_2:
              case ::SDLK_DOWN: {
                auto state = std::get<MenuState::menu_open>(
                    g_menu_state );
                if( !state.hover )
                  state.hover = g_items_from_menu[*menu].back();
                state.hover = util::find_subsequent_and_cycle(
                    g_items_from_menu[*menu], *state.hover );
                g_menu_state = state;
                log_menu_state();
                return true;
              }
              case ::SDLK_KP_8:
              case ::SDLK_UP: {
                auto state = std::get<MenuState::menu_open>(
                    g_menu_state );
                if( !state.hover )
                  state.hover = g_items_from_menu[*menu].front();
                state.hover = util::find_previous_and_cycle(
                    g_items_from_menu[*menu], *state.hover );
                g_menu_state = state;
                log_menu_state();
                return true;
              }
              default: break;
            }
          }
          return true;
        },
        []( input::mouse_wheel_event_t ) { return false; },
        []( input::mouse_move_event_t mv_event ) {
          event_counts()["menu mouse move"].tick();
          // Remove menu-hover by default and enable it again
          // below if the mouse if over a menu and menus are
          // closed.
          if( util::holds<MenuState::menus_closed>(
                  g_menu_state ) )
            g_menu_state = MenuState::menus_closed{};
          auto over_what = click_target( mv_event.pos );
          if( !over_what.has_value() ) return false;
          auto matcher = scelta::match<bool>(
              []( MouseOver::bar ) { return true; },
              []( MouseOver::divider desc ) {
                CHECK( util::holds<MenuState::menu_open>(
                           g_menu_state ) ||
                       util::holds<MenuState::item_click>(
                           g_menu_state ) );
                if( util::holds<MenuState::menu_open>(
                        g_menu_state ) ) {
                  g_menu_state = MenuState::menu_open{
                      desc.menu, /*hover=*/{}};
                }
                return true;
              },
              []( MouseOver::header header ) {
                if( util::holds<MenuState::menu_open>(
                        g_menu_state ) )
                  g_menu_state = MenuState::menu_open{
                      header.menu, /*hover=*/{}};
                if( util::holds<MenuState::menus_closed>(
                        g_menu_state ) )
                  g_menu_state = MenuState::menus_closed{
                      /*hover=*/header.menu};
                return true;
              },
              []( MouseOver::item item ) {
                CHECK( util::holds<MenuState::menu_open>(
                           g_menu_state ) ||
                       util::holds<MenuState::item_click>(
                           g_menu_state ) );
                if( util::holds<MenuState::menu_open>(
                        g_menu_state ) ) {
                  auto& o = std::get<MenuState::menu_open>(
                      g_menu_state );
                  CHECK( o.menu == g_item_to_menu[item.item] );
                  o.hover = {};
                  if( g_menu_items[item.item]
                          ->callbacks.enabled() )
                    o.hover = item.item;
                }
                return true;
              } )( //
              []( auto self, MouseOver::border border ) {
                // Delegate to the divider handler for now.
                return self( MouseOver_t{
                    MouseOver::divider{border.menu}} );
              } );
          return matcher( *over_what );
        },
        [&]( input::mouse_button_event_t b_event ) {
          auto over_what = click_target( b_event.pos );
          if( !over_what.has_value() ) {
            if( util::holds<MenuState::menu_open>(
                    g_menu_state ) ) {
              g_menu_state = MenuState::menus_closed{{}};
              log_menu_state();
              return true; // no click through
            }
            return false;
          }
          if( b_event.buttons ==
              input::e_mouse_button_event::left_down ) {
            auto matcher = scelta::match(
                []( MouseOver::bar ) {
                  g_menu_state = MenuState::menus_closed{{}};
                  log_menu_state();
                  return true;
                },
                []( MouseOver::border ) { return true; },
                []( MouseOver::divider ) { return true; },
                []( MouseOver::header header ) {
                  if( !is_menu_open( header.menu ) )
                    g_menu_state = MenuState::menu_open{
                        header.menu, /*hover=*/{}};
                  else
                    g_menu_state = MenuState::menus_closed{
                        /*hover=*/header.menu};
                  log_menu_state();
                  return true;
                },
                []( MouseOver::item ) { return true; } );
            return matcher( *over_what );
          }
          if( b_event.buttons ==
              input::e_mouse_button_event::left_up ) {
            auto matcher = scelta::match(
                []( MouseOver::bar ) {
                  g_menu_state = MenuState::menus_closed{{}};
                  log_menu_state();
                  return true;
                },
                []( MouseOver::border ) { return true; },
                []( MouseOver::divider ) { return true; },
                []( MouseOver::header ) { return true; },
                [&]( MouseOver::item item ) {
                  click_menu_item( item.item );
                  return true;
                } );
            return matcher( *over_what );
          }
          // `true` here because the mouse is over some menu ui
          // element (and that in turn is because
          // over_what.has_value() == true).
          return true;
        },
        []( input::mouse_drag_event_t ) {
          // The framework does not send us mouse drag events
          // directly; instead it uses the api methods on the
          // Plane class.
          SHOULD_NOT_BE_HERE;
          return false;
        } );
    return matcher( event );
  }

private:
  void click_menu_item( e_menu_item item ) {
    switch_v( g_menu_state ) {
      case_v( MenuState::item_click ) {
        // Already clicking, so do nothing. This can happen if a
        // menu item is clicked after it is already in the click
        // animation.
      }
      case_v( MenuState::menus_closed ) { SHOULD_NOT_BE_HERE; }
      case_v( MenuState::menus_hidden ) { SHOULD_NOT_BE_HERE; }
      case_v( MenuState::menu_open ) {
        // This check is important even if the code in this
        // module is structured in such a way that this function
        // is only called when the menu item has responded as
        // enabled. This is because it is possible that a menu
        // item might be disabled in the small amount of time
        // (within a frame) that the item is checked for
        // enablement and when this click is called.
        if( !g_menu_items[item]->callbacks.enabled() ) return;
        logger->info( "selected menu item `{}`", item );
        g_menu_state = MenuState::item_click{
            item, chrono::system_clock::now()};
        log_menu_state();
      }
      default_v;
    };
  }
};

MenuPlane g_menu_plane;

Plane* menu_plane() { return &g_menu_plane; }

} // namespace rn
