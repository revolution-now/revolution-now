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
#include "anim.hpp"
#include "compositor.hpp"
#include "config-files.hpp"
#include "error.hpp"
#include "frame.hpp"
#include "init.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "variant.hpp"

// Rds
#include "menu-impl.rds.hpp"

// Revolution Now (config)
#include "../config/rcl/palette.inl"
#include "../config/rcl/ui.inl"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/pixel.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/lambda.hpp"
#include "base/range-lite.hpp"
#include "base/to-str-ext-std.hpp"

// base-util
#include "base-util/algo.hpp"

using namespace std;

namespace rn {

namespace rl = ::base::rl;

namespace {
/****************************************************************
** Main Data Structures
*****************************************************************/
struct Menu {
  string name;
  bool   right_side;
  char   shortcut;
  int    name_width_pixels;
};

unordered_map<e_menu, Menu> g_menus{
    { e_menu::game, { "Game", false, 'G', {} } },
    { e_menu::view, { "View", false, 'V', {} } },
    { e_menu::orders, { "Orders", false, 'O', {} } },
    { e_menu::old_world, { "Old World", false, 'L', {} } },
    { e_menu::advisors, { "Advisors", false, 'A', {} } },
    { e_menu::music, { "Music", false, 'M', {} } },
    { e_menu::window, { "Window", false, 'W', {} } },
    { e_menu::debug, { "Debug", true, 'D', {} } },
    { e_menu::pedia, { "Revolopedia", true, 'R', {} } } };

unordered_map<e_menu_item, MenuItem::menu_clickable*>
                                           g_menu_items;
unordered_map<e_menu_item, e_menu>         g_item_to_menu;
unordered_map<e_menu, vector<e_menu_item>> g_items_from_menu;

#define ITEM( item, name )          \
  MenuItem::menu_clickable {        \
    e_menu_item::item, name, {}, {} \
  }

#define DIVIDER \
  MenuItem::menu_divider {}

/****************************************************************
** The Menus
*****************************************************************/
unordered_map<e_menu, vector<MenuItem_t>> g_menu_def{
    { e_menu::game,
      {
          ITEM( about, "About this Game" ),    //
          /***********/ DIVIDER, /***********/ //
          ITEM( save, "Save Game" ),           //
          ITEM( load, "Load Game" ),           //
          /***********/ DIVIDER, /***********/ //
          ITEM( next_turn, "Next Turn" ),      //
          /***********/ DIVIDER, /***********/ //
          ITEM( revolution, "REVOLUTION!" ),   //
          /***********/ DIVIDER, /***********/ //
          ITEM( retire, "Retire" ),            //
          ITEM( exit, "Exit to \"DOS\"" )      //
      } },
    { e_menu::view,
      {
          ITEM( zoom_in, "Zoom In" ),                      //
          ITEM( zoom_out, "Zoom Out" ),                    //
          /***********/ DIVIDER, /***********/             //
          ITEM( restore_zoom, "Zoom Default" ),            //
          /***********/ DIVIDER, /***********/             //
          ITEM( find_blinking_unit, "Find Blinking Unit" ) //
      } },
    { e_menu::orders,
      {
          ITEM( sentry, "Sentry" ),  //
          ITEM( fortify, "Fortify" ) //
      } },
    { e_menu::old_world,
      {
          ITEM( old_world_view, "View" ),  //
          ITEM( old_world_close, "Close" ) //
      } },
    { e_menu::advisors,
      {
          ITEM( military_advisor, "Military Advisor" ),   //
          ITEM( economics_advisor, "Economics Advisor" ), //
          ITEM( european_advisor, "Europian Advisor" )    //
      } },
    { e_menu::music,
      {
          ITEM( music_play, "Play" ),                     //
          ITEM( music_stop, "Stop" ),                     //
          ITEM( music_pause, "Pause" ),                   //
          ITEM( music_resume, "Resume" ),                 //
          /***********/ DIVIDER, /***********/            //
          ITEM( music_next, "Next Tune" ),                //
          ITEM( music_prev, "Previous Tune" ),            //
          /***********/ DIVIDER, /***********/            //
          ITEM( music_vol_up, "Volume Up" ),              //
          ITEM( music_vol_down, "Volume Down" ),          //
          /***********/ DIVIDER, /***********/            //
          ITEM( music_set_player, "Set Music Player..." ) //
      } },
    { e_menu::window,
      {
          ITEM( toggle_fullscreen, "Toggle Fullscreen" ), //
          ITEM( restore_window, "Restore" ),              //
          /***********/ DIVIDER, /***********/            //
          ITEM( scale_up, "Scale Up" ),                   //
          ITEM( scale_down, "Scale Down" ),               //
          ITEM( scale_optimal, "Scale Optimal" )          //
      } },
    { e_menu::debug,
      {
          ITEM( toggle_console, "Toggle Console" ) //
      } },
    { e_menu::pedia,
      {
          ITEM( units_help, "Units" ),                     //
          ITEM( terrain_help, "Terrain" ),                 //
          ITEM( founding_father_help, "Founding Fathers" ) //
      } } };

/****************************************************************
** Menu State
*****************************************************************/
MenuState_t g_menu_state{ MenuState::menus_closed{} };

void log_menu_state() {
  lg.trace( "g_menu_state: {}", g_menu_state );
}

/****************************************************************
** Querying State
*****************************************************************/
bool is_menu_open( e_menu menu ) {
  switch( g_menu_state.to_enum() ) {
    case MenuState::e::menus_hidden: {
      return false;
    }
    case MenuState::e::menus_closed: {
      return false;
    }
    case MenuState::e::item_click: {
      auto& click = g_menu_state.get<MenuState::item_click>();
      CHECK( g_item_to_menu.contains( click.item ) );
      return g_item_to_menu[click.item] == menu;
    }
    case MenuState::e::menu_open: {
      auto& o = g_menu_state.get<MenuState::menu_open>();
      return o.menu == menu;
    }
  }
}

maybe<e_menu> opened_menu() {
  if_get( g_menu_state, MenuState::menu_open, val ) {
    return val.menu;
  }
  return {};
}

bool is_menu_item_enabled_( e_menu_item item ) {
  CHECK( g_menu_items.contains( item ) );
  return g_menu_items[item]->callbacks.enabled();
}

auto is_menu_item_enabled =
    per_frame_memoize( is_menu_item_enabled_ );

/****************************************************************
** Colors
*****************************************************************/
auto const& menu_theme_color1 = gfx::pixel::banana();
auto const& menu_theme_color2 = gfx::pixel::wood();

namespace color::item::foreground {
auto disabled() {
  auto color = config_palette.grey.n88;
  color.a    = 200;
  return color;
}
} // namespace color::item::foreground

namespace color::menu::foreground {
// auto const& disabled = config_palette.grey.nA4;
} // namespace color::menu::foreground

H max_text_height() { return H{ 8 }; }

/****************************************************************
** Placement Geometry
*****************************************************************/
// The long, thin rectangle around the menu bar. This does not
// include the space that would be occupied by open menu bodies.
Rect menu_bar_rect() {
  UNWRAP_CHECK( res, compositor::section(
                         compositor::e_section::menu_bar ) );
  return res;
}

H menu_bar_height() { return menu_bar_rect().h; }

Delta menu_header_delta( e_menu menu ) {
  return Delta{ W{ g_menus[menu].name_width_pixels +
                   config_ui.menus.padding * 2_sx },
                menu_bar_height() };
}

X menu_header_x_pos( e_menu target ) {
  // TODO: simplify this since menus are not invisible anymore.
  CHECK( g_menus.contains( target ) );
  auto const& desc = g_menus[target];
  W           width_delta{ 0 };
  if( desc.right_side ) {
    width_delta = rl::rall( refl::enum_values<e_menu> )
                      .remove_if_L( !g_menus[_].right_side )
                      .take_while_incl_L( _ != target )
                      .map_L( menu_header_delta( _ ).w )
                      .intersperse( config_ui.menus.spacing )
                      .accumulate();
  } else {
    width_delta = rl::all( refl::enum_values<e_menu> )
                      .remove_if_L( g_menus[_].right_side )
                      .take_while_L( _ != target )
                      .map_L( menu_header_delta( _ ).w +
                              config_ui.menus.spacing )
                      .accumulate();
  }
  width_delta += config_ui.menus.first_menu_start;
  CHECK( width_delta >= 0_w );
  Rect rect = menu_bar_rect();
  return rect.x + ( !desc.right_side ? width_delta
                                     : rect.w - width_delta );
}

// Rectangle around a menu header.
Rect menu_header_rect( e_menu menu ) {
  return Rect::from(
      Coord{ menu_bar_rect().y, menu_header_x_pos( menu ) },
      menu_header_delta( menu ) );
}

Delta menu_header_text_size( e_menu menu ) {
  return Delta{ W{ g_menus[menu].name_width_pixels },
                max_text_height() };
}

Rect menu_header_text_rect( e_menu menu ) {
  Delta text_size = menu_header_text_size( menu );
  return Rect::from(
      centered( text_size, menu_header_rect( menu ) ),
      text_size );
}

// This is the width of the menu body not including the borders,
// which themselves occupy part of a tile.
W menu_body_width_inner( e_menu menu ) {
  W res{ 0 };
  for( auto const& item : g_items_from_menu[menu] )
    res = std::max( res,
                    W{ g_menu_items[item]->name_width_pixels } );
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
  H h{ 0 };
  CHECK( g_menu_def.contains( menu ) );
  for( auto const& item : g_menu_def[menu] ) {
    overload_visit(
        item,
        [&]( MenuItem::menu_divider const& ) {
          h += divider_height();
        },
        [&]( MenuItem::menu_clickable const& ) {
          h += max_text_height();
        } );
  }
  // round up to nearest multiple of 8, since that is the menu
  // tile width.
  if( h % 8_sy != 0_h ) h += ( 8_h - ( h % 8_sy ) );
  return h;
}

Delta menu_body_delta_inner( e_menu menu ) {
  return { menu_body_width_inner( menu ),
           menu_body_height_inner( menu ) };
}

Delta menu_body_delta( e_menu menu ) {
  return Delta{ 8_w, 8_h } + Delta{ 8_w, 8_h } +
         menu_body_delta_inner( menu );
}

Delta menu_item_delta( e_menu menu ) {
  return Delta{ menu_body_width_inner( menu ),
                max_text_height() };
}

Delta divider_delta( e_menu menu ) {
  return Delta{ divider_height(),
                menu_body_width_inner( menu ) };
}

Rect menu_body_rect_inner( e_menu menu ) {
  CHECK( g_menus.contains( menu ) );
  Coord pos;
  pos.y = menu_bar_rect().bottom_edge() + 8_h;
  if( g_menus[menu].right_side ) {
    pos.x = menu_header_x_pos( menu ) +
            menu_header_delta( menu ).w -
            menu_body_delta_inner( menu ).w;
  } else {
    pos.x = menu_header_x_pos( menu );
  }
  return Rect::from( pos, menu_body_delta_inner( menu ) );
}

Rect menu_body_rect( e_menu menu ) {
  CHECK( g_menus.contains( menu ) );
  Coord pos;
  pos.y = menu_bar_rect().bottom_edge();
  if( g_menus[menu].right_side )
    pos.x = menu_header_x_pos( menu ) +
            menu_header_delta( menu ).w + 8_w -
            menu_body_delta( menu ).w;
  else
    pos.x = menu_header_x_pos( menu ) - 8_w;
  return Rect::from( pos, menu_body_delta( menu ) );
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
maybe<e_menu_item> cursor_to_item( e_menu menu, H h ) {
  CHECK( g_menu_def.contains( menu ) );
  H pos{ 0 };
  for( auto const& item : g_menu_def[menu] ) {
    overload_visit(
        item,
        [&]( MenuItem::menu_divider const& ) {
          pos += divider_height();
        },
        [&]( MenuItem::menu_clickable const& ) {
          pos += max_text_height();
        } );
    if( pos > h ) {
      switch( item.to_enum() ) {
        case MenuItem::e::menu_divider: //
          return nothing;
        case MenuItem::e::menu_clickable: {
          auto& val = item.get<MenuItem::menu_clickable>();
          return val.item;
        }
      }
    }
  }
  return {};
}

// `cursor` is the screen position of the mouse cursor.
maybe<e_menu_item> cursor_to_item( e_menu menu, Coord cursor ) {
  if( !cursor.is_inside( menu_body_rect_inner( menu ) ) )
    return {};
  return cursor_to_item(
      menu, cursor.y - menu_body_rect_inner( menu ).top_edge() );
}

/****************************************************************
** Rendering Implmementation
*****************************************************************/
string_view name_for( e_menu menu ) {
  return g_menus[menu].name;
}

string_view name_for( e_menu_item item ) {
  return g_menu_items[item]->name;
}

// For either a menu header or item.
template<typename E>
void render_menu_element( rr::Renderer& renderer, Coord pos,
                          E element, gfx::pixel color ) {
  renderer.typer( "simple", pos, color )
      .write( name_for( element ) );
}

void render_divider( rr::Renderer& renderer, Coord pos,
                     e_menu menu ) {
  Delta      delta      = divider_delta( menu );
  gfx::pixel color_fore = menu_theme_color2.shaded( 3 );
  pos.y += delta.h / 2;
  pos.x += 2;
  renderer.painter().draw_horizontal_line( pos, delta.w._ - 5,
                                           color_fore );
}

void render_item_background_selected( rr::Renderer& renderer,
                                      Coord pos, e_menu menu ) {
  gfx::rect background = gfx::rect{
      .origin = pos, .size = menu_item_delta( menu ) };
  renderer.painter().draw_solid_rect( background,
                                      menu_theme_color1 );
}

void render_menu_header_background( rr::Painter& painter,
                                    e_menu menu, bool active ) {
  // TODO: use render_sprite_section here.
  Rect  header  = menu_header_rect( menu );
  Coord where   = header.upper_left();
  Rect  section = Rect::from( Coord{}, header.delta() );
  if( active )
    render_sprite_section( painter, e_tile::menu_item_sel_back,
                           where, section );
  else
    render_sprite_section( painter, e_tile::menu_hdr_sel_back,
                           where, section );
}

void render_open_menu( rr::Renderer& renderer, Coord pos,
                       e_menu             menu,
                       maybe<e_menu_item> subject ) {
  rr::Painter painter = renderer.painter();
  if( subject.has_value() ) {
    CHECK( g_item_to_menu[*subject] == menu );
  }

  render_rect_of_sprites_with_border(
      painter,                                       //
      pos,                                           //
      menu_body_delta( menu ) / Scale{ 8_sx, 8_sy }, //
      e_tile::menu_body,                             //
      e_tile::menu_top,                              //
      e_tile::menu_bottom,                           //
      e_tile::menu_left,                             //
      e_tile::menu_right,                            //
      e_tile::menu_top_left,                         //
      e_tile::menu_top_right,                        //
      e_tile::menu_bottom_left,                      //
      e_tile::menu_bottom_right                      //
  );

  pos.x += 8_w;
  pos.y += 8_h;

  for( auto const& item : g_menu_def[menu] ) {
    overload_visit(
        item,
        [&]( MenuItem::menu_divider ) {
          render_divider( renderer, pos, menu );
          pos += divider_height();
        },
        [&]( MenuItem::menu_clickable const& clickable ) {
          bool on = ( clickable.item == subject );
          if( on )
            render_item_background_selected( renderer, pos,
                                             menu );
          gfx::pixel foreground_color =
              !is_menu_item_enabled( clickable.item )
                  ? color::item::foreground::disabled()
              : on ? menu_theme_color2
                   : menu_theme_color1;
          render_menu_element(
              renderer, pos + config_ui.menus.padding - 1_h,
              clickable.item, foreground_color );
          pos += max_text_height();
        } );
  }
}

void render_menu_bar( rr::Renderer& renderer ) {
  // Render the "wood" panel. Start from the left edge of the
  // panel so that we get a continuous wood texture between the
  // two. Also, put the y position such that the menu bar gets
  // the bottom portion of the texture, again so that it will be
  // continuous with that panel.
  auto panel_upper_left =
      compositor::section( compositor::e_section::panel )
          .value_or( Rect{} )
          .upper_left();
  auto        bar_rect   = menu_bar_rect();
  Delta       wood_size  = sprite_size( e_tile::wood_middle );
  Coord       start      = panel_upper_left - wood_size.h;
  auto        wood_width = wood_size.w;
  rr::Painter painter    = renderer.painter();
  for( Coord c = start; c.x >= 0_x - wood_width;
       c -= wood_width )
    render_sprite( painter, e_tile::wood_middle, c );
  for( Coord c = start; c.x < bar_rect.right_edge();
       c += wood_width )
    render_sprite( painter, e_tile::wood_middle, c );

  for( auto menu : refl::enum_values<e_menu> ) {
    auto  rect                  = menu_header_rect( menu );
    Coord background_upper_left = rect.upper_left();
    Coord foreground_upper_left =
        menu_header_text_rect( menu ).upper_left();
    // Given `menu`, this matcher visits the global menu state
    // and renders the foreground/background for that menu. Use a
    // struct to visit so that we can get recursive visitation.
    struct {
      // These would be the "lambda captures".
      e_menu const& menu;
      Coord const&  background_upper_left;
      Coord const&  foreground_upper_left;
      rr::Renderer& renderer;

      void operator()( MenuState::menus_hidden ) const {}
      void operator()( MenuState::menus_closed closed ) const {
        rr::Painter painter = renderer.painter();
        if( menu == closed.hover )
          render_menu_header_background( painter, menu,
                                         /*active=*/false );
        render_menu_element( renderer, foreground_upper_left,
                             menu, menu_theme_color1 );
      }
      void operator()( MenuState::item_click const& ic ) const {
        // Just forward this to the MenuState::menu_open.
        CHECK( g_item_to_menu.contains( ic.item ) );
        this->operator()(
            MenuState::menu_open{ g_item_to_menu[ic.item],
                                  /*hover=*/{} } );
      }
      void operator()( MenuState::menu_open const& o ) const {
        if( o.menu != menu )
          return this->operator()( MenuState::menus_closed{} );
        rr::Painter painter = renderer.painter();
        render_menu_header_background( painter, menu,
                                       /*active=*/true );
        render_menu_element( renderer, foreground_upper_left,
                             menu, menu_theme_color2 );
      }
    } matcher{ menu, background_upper_left,
               foreground_upper_left, renderer };
    std::visit( matcher, g_menu_state );
  }
}

/****************************************************************
** Input Implementation
*****************************************************************/
maybe<MouseOver_t> click_target( Coord screen_coord ) {
  using res_t = maybe<MouseOver_t>;
  // menu. Use a struct to visit so that we can get recursive
  // visitation.
  struct {
    // These would be the "lambda captures".
    Coord const& screen_coord;

    res_t operator()( MenuState::menus_hidden ) const {
      return res_t{};
    }
    res_t operator()( MenuState::menus_closed ) const {
      for( auto menu : refl::enum_values<e_menu> )
        if( screen_coord.is_inside( menu_header_rect( menu ) ) )
          return res_t{ MouseOver::header{ menu } };
      if( screen_coord.is_inside( menu_bar_rect() ) )
        return res_t{ MouseOver::bar{} };
      return res_t( nothing );
    }
    res_t operator()( MenuState::item_click const& ic ) const {
      // Just forward this to the MenuState::menu_open.
      CHECK( g_item_to_menu.contains( ic.item ) );
      return ( *this )( MenuState::menu_open{
          g_item_to_menu[ic.item], /*hover=*/{} } );
    }
    res_t operator()( MenuState::menu_open const& o ) const {
      auto closed = ( *this )( MenuState::menus_closed{} );
      if( closed ) return res_t{ closed };
      if( !screen_coord.is_inside(
              menu_body_clickable_area( o.menu ) ) )
        return res_t( nothing );
      // The cursor is over a non-transparent part of the open
      // menu.
      if( !screen_coord.is_inside(
              menu_body_rect_inner( o.menu ) ) )
        return res_t{ MouseOver::border{ o.menu } };
      // The cursor is over the inner menu body, so at this
      // point its either over an item or a divider.
      auto maybe_item = cursor_to_item( o.menu, screen_coord );
      if( !maybe_item.has_value() )
        return res_t{ MouseOver::divider{ o.menu } };
      // Finally, we are over an item.
      return res_t{ MouseOver::item{ *maybe_item } };
    }
  } matcher{ screen_coord };
  return std::visit( matcher, g_menu_state );
}

/****************************************************************
** Top-level Render Method
*****************************************************************/
void render_menus( rr::Renderer& renderer ) {
  render_menu_bar( renderer );
  // maybe render open menu.
  overload_visit(
      g_menu_state, []( MenuState::menus_hidden ) {},
      [&]( MenuState::menus_closed ) {},
      [&]( MenuState::item_click const& ic ) {
        auto menu = g_item_to_menu[ic.item];
        CHECK( g_item_to_menu.contains( ic.item ) );
        Coord pos = menu_body_rect( menu ).upper_left();
        render_open_menu( renderer, pos, menu, ic.item );
      },
      [&]( MenuState::menu_open const& o ) {
        Coord pos = menu_body_rect( o.menu ).upper_left();
        render_open_menu( renderer, pos, o.menu, o.hover );
      } );
}

/****************************************************************
** Registration
*****************************************************************/
auto& on_click_handlers() {
  static unordered_map<e_menu_item, std::function<void( void )>>
      m;
  return m;
}

auto& is_enabled_handlers() {
  static unordered_map<e_menu_item, std::function<bool( void )>>
      m;
  return m;
}

/****************************************************************
** Initialization
*****************************************************************/
void init_menus() {
  // Check that all menus have descriptors.
  for( auto menu : refl::enum_values<e_menu> ) {
    CHECK( g_menus.contains( menu ) );
    CHECK( g_menu_def.contains( menu ) );
  }

  // Populate the e_menu_item maps and verify no duplicates.
  for( auto& [menu, items] : g_menu_def ) {
    for( auto& item_desc : items ) {
      if( holds<MenuItem::menu_divider>( item_desc ) ) continue;
      CHECK( holds<MenuItem::menu_clickable>( item_desc ) );
      auto& clickable =
          get<MenuItem::menu_clickable>( item_desc );
      g_items_from_menu[menu].push_back( clickable.item );
      CHECK( !g_menu_items.contains( clickable.item ) );
      g_menu_items[clickable.item] = &clickable;
      CHECK( !g_item_to_menu.contains( clickable.item ) );
      g_item_to_menu[clickable.item] = menu;
    }
  }

  // Check that g_items_from_menu is populated.
  for( auto menu : refl::enum_values<e_menu> ) {
    CHECK( g_items_from_menu.contains( menu ) );
  }

  // Check that all menus have at least one item.
  for( auto menu : refl::enum_values<e_menu> ) {
    CHECK( g_items_from_menu[menu].size() > 0 );
  }

  // Check that all menus have unique shortcut keys and that the
  // menu header name contains the shortcut key (in either
  // uppercase or lowercase.
  unordered_set<char> keys;
  for( auto menu : refl::enum_values<e_menu> ) {
    char key = tolower( g_menus[menu].shortcut );
    CHECK( !keys.contains( key ),
           "multiple menus have `{}` as a shortcut key", key );
    keys.insert( key );
    string_view name = g_menus[menu].name;
    CHECK( name.find( tolower( key ) ) != string_view::npos ||
               name.find( toupper( key ) ) != string_view::npos,
           "menu `{}` does not contain shortcut key `{}`", name,
           key );
  }

  // Check that all e_menu_items are in a menu.
  for( auto item : refl::enum_values<e_menu_item> ) {
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

  // Populate text widths of menu and menu item names.
  int kCharWidth = rr::rendered_text_line_size_pixels( "X" ).w;
  for( auto menu : refl::enum_values<e_menu> )
    g_menus[menu].name_width_pixels =
        g_menus[menu].name.size() * kCharWidth;
  for( auto item : refl::enum_values<e_menu_item> )
    g_menu_items[item]->name_width_pixels =
        g_menu_items[item]->name.size() * kCharWidth;

  // Check that all e_menu_items have registered handlers.
  for( auto item : refl::enum_values<e_menu_item> ) {
    auto const& desc = *g_menu_items[item];
    CHECK( item == desc.item );
    CHECK( desc.name.size() > 0 );
    CHECK( desc.callbacks.on_click && desc.callbacks.enabled,
           "the menu item `{}` does not have callback handlers "
           "registered",
           item );
  }
}

void cleanup_menus() {}

REGISTER_INIT_ROUTINE( menus );

/****************************************************************
** The Menu Plane
*****************************************************************/
struct MenuPlane : public Plane {
  MenuPlane() = default;
  bool covers_screen() const override { return false; }
  void advance_state() override {
    if_get( g_menu_state, MenuState::item_click, val ) {
      // We must cache this item before changing the menu state
      // since `val` will be invalidated once we do so.
      e_menu_item item = val.item;
      g_menu_state     = MenuState::menus_closed{ /*hover=*/{} };
      DCHECK( g_menu_items[item], "g_menu_items[{}] is nullptr.",
              val.item );
      g_menu_items[item]->callbacks.on_click();
    }
  }
  Plane::e_accept_drag can_drag( input::e_mouse_button button,
                                 Coord origin ) override {
    if( !click_target( origin ).has_value() )
      return Plane::e_accept_drag::no;
    // Convert drags to mouse motion events.
    return ( button == input::e_mouse_button::l )
               ? Plane::e_accept_drag::motion
               : Plane::e_accept_drag::swallow;
  }
  void draw( rr::Renderer& renderer ) const override {
    render_menus( renderer );
  }
  e_input_handled input( input::event_t const& event ) override {
    return overload_visit(
        event,
        []( input::unknown_event_t ) {
          return e_input_handled::no;
        },
        []( input::quit_event_t ) {
          return e_input_handled::no;
        },
        []( input::win_event_t ) { return e_input_handled::no; },
        [&]( input::key_event_t const& key_event ) {
          if( holds<MenuState::item_click>( g_menu_state ) )
            // If we are in the middle of a click process then
            // let it finish before handling anymore keys.
            return e_input_handled::no;
          auto menu = opened_menu();
          // `is_alt` decides whether the pressed key is an alt,
          // not whether it is being held down while another key
          // is pressed ( the latter would be
          // keyevent.mod.alt_down).
          bool is_alt = ( key_event.keycode == ::SDLK_LALT ||
                          key_event.keycode == ::SDLK_RALT );
          if( !menu.has_value() &&
              key_event.change == input::e_key_change::down &&
              is_alt ) {
            // Menus are closed and the user has pressed an alt
            // key, so add highlighting to the first menu header
            // that is visible (if any).
            auto maybe_first_menu =
                rl::all( refl::enum_values<e_menu> ).head();
            if( maybe_first_menu.has_value() ) {
              g_menu_state = MenuState::menus_closed{
                  /*hover=*/*maybe_first_menu };
              return e_input_handled::yes;
            }
          }
          if( !menu.has_value() &&
              key_event.change == input::e_key_change::up &&
              is_alt ) {
            // Menus are closed and the user is releasing an alt
            // key, so remove any highlighting from the menu
            // headers.
            g_menu_state =
                MenuState::menus_closed{ /*hover=*/{} };
            return e_input_handled::yes;
          }
          if( menu.has_value() &&
              key_event.change == input::e_key_change::down &&
              is_alt ) {
            // Menus are open and the user is pressing an alt
            // key, so close menus.
            g_menu_state =
                MenuState::menus_closed{ /*hover=*/{} };
            return e_input_handled::yes;
          }
          // Check for an alt-shortcut key to open a menu.
          if( key_event.change == input::e_key_change::down &&
              key_event.mod.alt_down ) {
            for( auto menu : refl::enum_values<e_menu> ) {
              auto const& menu_desc = g_menus[menu];
              if( key_event.keycode ==
                  tolower( menu_desc.shortcut ) ) {
                if( !is_menu_open( menu ) )
                  g_menu_state =
                      MenuState::menu_open{ menu, /*hover=*/{} };
                else
                  g_menu_state =
                      MenuState::menus_closed{ /*hover=*/menu };
                log_menu_state();
                return e_input_handled::yes;
              }
            }
          }
          if( !menu ) return e_input_handled::no;
          if( key_event.change == input::e_key_change::down ) {
            switch( key_event.keycode ) {
              case ::SDLK_RETURN:
              case ::SDLK_KP_5:
              case ::SDLK_KP_ENTER:
                if_get( g_menu_state, MenuState::menu_open,
                        val ) {
                  if( val.hover.has_value() ) {
                    click_menu_item( val.hover.value() );
                    log_menu_state();
                  }
                }
                return e_input_handled::yes;
              case ::SDLK_ESCAPE:
                g_menu_state = MenuState::menus_closed{ {} };
                log_menu_state();
                return e_input_handled::yes;
              case ::SDLK_KP_4:
              case ::SDLK_LEFT: {
                menu = util::find_previous_and_cycle(
                    refl::enum_values<e_menu>, *menu );
                CHECK( menu );
                g_menu_state =
                    MenuState::menu_open{ *menu, /*hover=*/{} };
                log_menu_state();
                return e_input_handled::yes;
              }
              case ::SDLK_KP_6:
              case ::SDLK_RIGHT: {
                menu = util::find_subsequent_and_cycle(
                    refl::enum_values<e_menu>, *menu );
                CHECK( menu );
                g_menu_state =
                    MenuState::menu_open{ *menu, /*hover=*/{} };
                log_menu_state();
                return e_input_handled::yes;
              }
              case ::SDLK_KP_2:
              case ::SDLK_DOWN: {
                auto state = std::get<MenuState::menu_open>(
                    g_menu_state );
                if( !state.hover )
                  state.hover = g_items_from_menu[*menu].back();
                auto start = state.hover;
                do {
                  state.hover = util::find_subsequent_and_cycle(
                      g_items_from_menu[*menu], *state.hover );
                  if( state.hover == start )
                    // This is just a safe guard in case somehow
                    // there are no menu items enabled at this
                    // point, even though that should never
                    // happen (it would cause an infinite loop).
                    break;
                } while( !is_menu_item_enabled( *state.hover ) );
                g_menu_state = state;
                log_menu_state();
                return e_input_handled::yes;
              }
              case ::SDLK_KP_8:
              case ::SDLK_UP: {
                auto state = std::get<MenuState::menu_open>(
                    g_menu_state );
                if( !state.hover )
                  state.hover = g_items_from_menu[*menu].front();
                auto start = state.hover;
                do {
                  state.hover = util::find_previous_and_cycle(
                      g_items_from_menu[*menu], *state.hover );
                  if( state.hover == start )
                    // This is just a safe guard in case somehow
                    // there are no menu items enabled at this
                    // point, even though that should never
                    // happen (it would cause an infinite loop).
                    break;
                } while( !is_menu_item_enabled( *state.hover ) );
                g_menu_state = state;
                log_menu_state();
                return e_input_handled::yes;
              }
              default: break;
            }
          }
          return e_input_handled::yes;
        },
        []( input::mouse_wheel_event_t ) {
          return e_input_handled::no;
        },
        []( input::mouse_move_event_t mv_event ) {
          // Remove menu-hover by default and enable it again
          // below if the mouse if over a menu and menus are
          // closed.
          if( holds<MenuState::menus_closed>( g_menu_state ) )
            g_menu_state = MenuState::menus_closed{};
          auto over_what = click_target( mv_event.pos );
          if( !over_what.has_value() )
            return e_input_handled::no;
          struct {
            e_input_handled operator()( MouseOver::bar ) {
              return e_input_handled::yes;
            }
            e_input_handled operator()(
                MouseOver::divider desc ) {
              CHECK(
                  holds<MenuState::menu_open>( g_menu_state ) ||
                  holds<MenuState::item_click>( g_menu_state ) );
              if( holds<MenuState::menu_open>( g_menu_state ) ) {
                g_menu_state = MenuState::menu_open{
                    desc.menu, /*hover=*/{} };
              }
              return e_input_handled::yes;
            }
            e_input_handled operator()(
                MouseOver::header header ) {
              if( holds<MenuState::menu_open>( g_menu_state ) )
                g_menu_state =
                    MenuState::menu_open{ header.menu,
                                          /*hover=*/{} };
              if( holds<MenuState::menus_closed>(
                      g_menu_state ) )
                g_menu_state = MenuState::menus_closed{
                    /*hover=*/header.menu };
              return e_input_handled::yes;
            }
            e_input_handled operator()( MouseOver::item item ) {
              CHECK(
                  holds<MenuState::menu_open>( g_menu_state ) ||
                  holds<MenuState::item_click>( g_menu_state ) );
              if( holds<MenuState::menu_open>( g_menu_state ) ) {
                auto& o = std::get<MenuState::menu_open>(
                    g_menu_state );
                CHECK( o.menu == g_item_to_menu[item.item] );
                o.hover = {};
                if( is_menu_item_enabled( item.item ) )
                  o.hover = item.item;
              }
              return e_input_handled::yes;
            }
            e_input_handled operator()(
                MouseOver::border border ) {
              // Delegate to the divider handler for now.
              return ( *this )(
                  MouseOver::divider{ border.menu } );
            }
          } matcher;
          return std::visit( matcher, *over_what );
        },
        [&]( input::mouse_button_event_t b_event ) {
          auto over_what = click_target( b_event.pos );
          if( !over_what.has_value() ) {
            if( holds<MenuState::menu_open>( g_menu_state ) ) {
              g_menu_state = MenuState::menus_closed{ {} };
              log_menu_state();
              return e_input_handled::yes; // no click through
            }
            return e_input_handled::no;
          }
          if( b_event.buttons ==
              input::e_mouse_button_event::left_down ) {
            return overload_visit(
                *over_what,
                []( MouseOver::bar ) {
                  g_menu_state = MenuState::menus_closed{ {} };
                  log_menu_state();
                  return e_input_handled::yes;
                },
                []( MouseOver::border ) {
                  return e_input_handled::yes;
                },
                []( MouseOver::divider ) {
                  return e_input_handled::yes;
                },
                []( MouseOver::header header ) {
                  if( !is_menu_open( header.menu ) )
                    g_menu_state =
                        MenuState::menu_open{ header.menu,
                                              /*hover=*/{} };
                  else
                    g_menu_state = MenuState::menus_closed{
                        /*hover=*/header.menu };
                  log_menu_state();
                  return e_input_handled::yes;
                },
                []( MouseOver::item ) {
                  return e_input_handled::yes;
                } );
          }
          if( b_event.buttons ==
              input::e_mouse_button_event::left_up ) {
            return overload_visit(
                *over_what,
                []( MouseOver::bar ) {
                  g_menu_state = MenuState::menus_closed{ {} };
                  log_menu_state();
                  return e_input_handled::yes;
                },
                []( MouseOver::border ) {
                  return e_input_handled::yes;
                },
                []( MouseOver::divider ) {
                  return e_input_handled::yes;
                },
                []( MouseOver::header ) {
                  return e_input_handled::yes;
                },
                [&]( MouseOver::item item ) {
                  click_menu_item( item.item );
                  return e_input_handled::yes;
                } );
          }
          // `true` here because the mouse is over some menu ui
          // element (and that in turn is because
          // over_what.has_value() == true).
          return e_input_handled::yes;
        },
        []( input::mouse_drag_event_t ) {
          // The framework does not send us mouse drag events
          // directly; instead it uses the api methods on the
          // Plane class.
          SHOULD_NOT_BE_HERE;
          return e_input_handled::no;
        } );
  }

 private:
  void click_menu_item( e_menu_item item ) {
    switch( g_menu_state.to_enum() ) {
      case MenuState::e::item_click: {
        // Already clicking, so do nothing. This can happen if a
        // menu item is clicked after it is already in the click
        // animation.
        break;
      }
      case MenuState::e::menus_closed: {
        SHOULD_NOT_BE_HERE;
        break;
      }
      case MenuState::e::menus_hidden: {
        SHOULD_NOT_BE_HERE;
        break;
      }
      case MenuState::e::menu_open: {
        // This check is important even if the code in this
        // module is structured in such a way that this function
        // is only called when the menu item has responded as
        // enabled. This is because it is possible that a menu
        // item might be disabled in the small amount of time
        // (within a frame) that the item is checked for
        // enablement and when this click is called. That's also
        // why we don't call the (memoized) is_menu_item_enabled.
        if( !g_menu_items[item]->callbacks.enabled() ) break;
        lg.info( "selected menu item `{}`", item );
        g_menu_state = MenuState::item_click{
            item, chrono::system_clock::now() };
        log_menu_state();
        break;
      }
    };
  }
};

MenuPlane g_menu_plane;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
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

H menu_height() { return menu_bar_height(); }

Plane* menu_plane() { return &g_menu_plane; }

} // namespace rn
