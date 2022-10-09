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
#include "error.hpp"
#include "frame.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "variant.hpp"

// Rds
#include "menu-impl.rds.hpp"

// config
#include "config/ui.rds.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/pixel.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/lambda.hpp"
#include "base/range-lite.hpp"
#include "base/to-str-ext-std.hpp"

// base-util
#include "base-util/algo.hpp"

// C++ standard library
#include <stack>

using namespace std;

namespace rn {

namespace rl = ::base::rl;

namespace {

/****************************************************************
** Top level menu structure.
*****************************************************************/
struct Menu {
  string name;
  bool   right_side;
  char   shortcut;
};

refl::enum_map<e_menu, Menu> const g_menus{
    { e_menu::game, { "Game", false, 'G' } },
    { e_menu::view, { "View", false, 'V' } },
    { e_menu::orders, { "Orders", false, 'O' } },
    { e_menu::advisors, { "Advisors", false, 'A' } },
    { e_menu::music, { "Music", false, 'M' } },
    { e_menu::window, { "Window", false, 'W' } },
    { e_menu::cheat, { "Cheat", true, 'C' } },
    { e_menu::pedia, { "Revolopedia", true, 'R' } } };

/****************************************************************
** Menu contents.
*****************************************************************/
#define ITEM( item, name ) \
  MenuItem::menu_clickable { e_menu_item::item, name }

#define DIVIDER \
  MenuItem::menu_divider {}

refl::enum_map<e_menu, vector<MenuItem_t>> const g_menu_def{
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
          ITEM( zoom_in, "Zoom In" ),                       //
          ITEM( zoom_out, "Zoom Out" ),                     //
          ITEM( restore_zoom, "Zoom Default" ),             //
          /***********/ DIVIDER, /***********/              //
          ITEM( find_blinking_unit, "Find Blinking Unit" ), //
          /***********/ DIVIDER, /***********/              //
          ITEM( harbor_view, "European Status" ),           //
          /***********/ DIVIDER, /***********/              //
          ITEM( hidden_terrain, "Show Hidden Terrain" )     //
      } },
    { e_menu::orders,
      {
          ITEM( sentry, "Sentry" ),             //
          ITEM( fortify, "Fortify" ),           //
          ITEM( road, "Build Road" ),           //
          ITEM( plow, "Clear Forest/Plow" ),    //
          ITEM( dump, "Dump Cargo Overboard" ), //
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
    { e_menu::cheat,
      {
          ITEM( cheat_reveal_map, "Reveal Map" ),             //
          ITEM( cheat_map_editor, "Enter Map Editor" ),       //
          /***********/ DIVIDER, /***********/                //
          ITEM( cheat_edit_fathers, "Edit Founding Fathers" ) //
      } },
    { e_menu::pedia,
      {
          ITEM( units_help, "Units" ),                     //
          ITEM( terrain_help, "Terrain" ),                 //
          ITEM( founding_father_help, "Founding Fathers" ) //
      } } };

} // namespace

/****************************************************************
** MenuPlane::Impl
*****************************************************************/
struct MenuPlane::Impl : public Plane {
  // State
  refl::enum_map<e_menu, int>      menu_name_width_pixels_;
  refl::enum_map<e_menu_item, int> menu_item_name_width_pixels_;

  refl::enum_map<e_menu_item, MenuItem::menu_clickable const*>
                                              menu_items_;
  refl::enum_map<e_menu_item, e_menu>         item_to_menu_;
  refl::enum_map<e_menu, vector<e_menu_item>> items_from_menu_;

  MenuState_t menu_state_{ MenuState::menus_closed{} };

  refl::enum_map<e_menu_item, stack<Plane*>> handlers_;

  Impl() {
    // TODO: probably should only do these things once.

    // Populate the e_menu_item maps and verify no duplicates.
    for( auto& [menu, items] : g_menu_def ) {
      for( auto& item_desc : items ) {
        if( holds<MenuItem::menu_divider>( item_desc ) )
          continue;
        CHECK( holds<MenuItem::menu_clickable>( item_desc ) );
        auto& clickable =
            get<MenuItem::menu_clickable>( item_desc );
        items_from_menu_[menu].push_back( clickable.item );
        menu_items_[clickable.item]   = &clickable;
        item_to_menu_[clickable.item] = menu;
      }
    }

    // Check that all menus have at least one item.
    for( auto menu : refl::enum_values<e_menu> ) {
      CHECK( items_from_menu_[menu].size() > 0 );
    }

    // Check that all menus have unique shortcut keys and that
    // the menu header name contains the shortcut key (in either
    // uppercase or lowercase.
    unordered_set<char> keys;
    for( auto menu : refl::enum_values<e_menu> ) {
      char key = tolower( g_menus[menu].shortcut );
      CHECK( !keys.contains( key ),
             "multiple menus have `{}` as a shortcut key", key );
      keys.insert( key );
      string_view name = g_menus[menu].name;
      CHECK(
          name.find( tolower( key ) ) != string_view::npos ||
              name.find( toupper( key ) ) != string_view::npos,
          "menu `{}` does not contain shortcut key `{}`", name,
          key );
    }

    // Check that all e_menu_items are in a menu.
    for( auto item : refl::enum_values<e_menu_item> ) {
      CHECK( menu_items_[item] != nullptr );
    }

    // Populate text widths of menu and menu item names.
    int kCharWidth = rr::rendered_text_line_size_pixels( "X" ).w;
    for( auto menu : refl::enum_values<e_menu> )
      menu_name_width_pixels_[menu] =
          g_menus[menu].name.size() * kCharWidth;
    for( auto item : refl::enum_values<e_menu_item> )
      menu_item_name_width_pixels_[item] =
          menu_items_[item]->name.size() * kCharWidth;
  }

  /****************************************************************
  ** Menu State
  *****************************************************************/

  void log_menu_state() const {
    lg.trace( "menu_state_: {}", menu_state_ );
  }

  /****************************************************************
  ** Querying State
  *****************************************************************/
  bool is_menu_open( e_menu menu ) const {
    switch( menu_state_.to_enum() ) {
      case MenuState::e::menus_hidden: {
        return false;
      }
      case MenuState::e::menus_closed: {
        return false;
      }
      case MenuState::e::item_click: {
        auto& click = menu_state_.get<MenuState::item_click>();
        return item_to_menu_[click.item] == menu;
      }
      case MenuState::e::menu_open: {
        auto& o = menu_state_.get<MenuState::menu_open>();
        return o.menu == menu;
      }
    }
  }

  maybe<e_menu> opened_menu() const {
    if_get( menu_state_, MenuState::menu_open, val ) {
      return val.menu;
    }
    return {};
  }

  /****************************************************************
  ** Colors
  *****************************************************************/
  inline static gfx::pixel const menu_theme_color1 =
      gfx::pixel::banana();
  inline static gfx::pixel const menu_theme_color2 =
      gfx::pixel::wood();

  auto foreground_disabled_color() const {
    static auto color =
        gfx::pixel{ .r = 0x88, .g = 0x88, .b = 0x88, .a = 255 };
    color.a = 200;
    return color;
  }

  H max_text_height() const { return H{ 8 }; }

  /****************************************************************
  ** Menu Bar
  *****************************************************************/
  // The long, thin rectangle around the menu bar. This does not
  // include the space that would be occupied by open menu
  // bodies.
  Rect menu_bar_rect() const {
    UNWRAP_CHECK( res, compositor::section(
                           compositor::e_section::menu_bar ) );
    return res;
  }

  H menu_bar_height() const { return menu_bar_rect().h; }

  /****************************************************************
  ** Menu Headers
  *****************************************************************/
  Delta menu_header_delta( e_menu menu ) const {
    return Delta{ .w = menu_name_width_pixels_[menu] +
                       config_ui.menus.padding_x * 2,
                  .h = menu_bar_height() - 4 };
  }

  X menu_header_x_pos( e_menu target ) const {
    // TODO: simplify this since menus are not invisible anymore.
    auto const& desc = g_menus[target];
    W           width_delta{ 0 };
    if( desc.right_side ) {
      width_delta = rl::rall( refl::enum_values<e_menu> )
                        .remove_if_L( !g_menus[_].right_side )
                        .take_while_incl_L( _ != target )
                        .map_L( menu_header_delta( _ ).w )
                        .intersperse( config_ui.menus.spacing_x )
                        .accumulate();
    } else {
      width_delta = rl::all( refl::enum_values<e_menu> )
                        .remove_if_L( g_menus[_].right_side )
                        .take_while_L( _ != target )
                        .map_L( menu_header_delta( _ ).w +
                                config_ui.menus.spacing_x )
                        .accumulate();
    }
    width_delta += config_ui.menus.first_menu_start_x_offset;
    CHECK( width_delta >= 0 );
    Rect rect = menu_bar_rect();
    return rect.x + ( !desc.right_side ? width_delta
                                       : rect.w - width_delta );
  }

  // Rectangle around a menu header.
  Rect menu_header_rect( e_menu menu ) const {
    return Rect::from( Coord{ .x = menu_header_x_pos( menu ),
                              .y = menu_bar_rect().y + 2 },
                       menu_header_delta( menu ) );
  }

  Delta menu_header_text_size( e_menu menu ) const {
    return Delta{ .w = W{ menu_name_width_pixels_[menu] },
                  .h = max_text_height() };
  }

  Rect menu_header_text_rect( e_menu menu ) const {
    Delta text_size = menu_header_text_size( menu );
    return Rect::from(
        centered( text_size, menu_header_rect( menu ) ),
        text_size );
  }

  /****************************************************************
  ** Menu Body
  *****************************************************************/
  // This is the width of the menu body not including the
  // borders, which themselves occupy part of a tile.
  W menu_body_width_inner( e_menu menu ) const {
    W res{ 0 };
    for( auto const& item : items_from_menu_[menu] )
      res = std::max( res,
                      W{ menu_item_name_width_pixels_[item] } );
    // At this point, res holds the width of the largest rendered
    // text texture in this menu.  Now add padding on each side:
    res += config_ui.menus.padding_x * 2;
    res = clamp( res, config_ui.menus.body_min_width, 1000000 );
    // round up to nearest multiple of 8, since that is the menu
    // tile width.
    if( res % 8 != 0 ) res += ( 8 - ( res % 8 ) );
    // Sanity check
    CHECK( res > 0 && res < 2000 );
    return res;
  }

  H menu_item_height() const {
    return max_text_height() +
           config_ui.menus.item_vertical_padding * 2;
  }

  Delta menu_item_delta( e_menu menu ) const {
    return Delta{ .w = menu_body_width_inner( menu ),
                  .h = menu_item_height() };
  }

  H divider_height() const { return menu_item_height() / 2; }

  Delta divider_delta( e_menu menu ) const {
    return Delta{ .w = menu_body_width_inner( menu ),
                  .h = divider_height() };
  }

  // This is the width of the menu body not including the
  // borders, which themselves occupy part of a tile.
  H menu_body_height_inner( e_menu menu ) const {
    H h{ 0 };
    for( auto const& item : g_menu_def[menu] ) {
      overload_visit(
          item,
          [&]( MenuItem::menu_divider const& ) {
            h += divider_height();
          },
          [&]( MenuItem::menu_clickable const& ) {
            h += menu_item_height();
          } );
    }
    // round up to nearest multiple of 8, since that is the menu
    // tile width.
    if( h % 8 != 0 ) h += ( 8 - ( h % 8 ) );
    return h;
  }

  Delta menu_body_delta_inner( e_menu menu ) const {
    return { menu_body_width_inner( menu ),
             menu_body_height_inner( menu ) };
  }

  Delta menu_body_delta( e_menu menu ) const {
    return Delta{ .w = 8, .h = 8 } + Delta{ .w = 8, .h = 8 } +
           menu_body_delta_inner( menu );
  }

  Rect menu_body_rect_inner( e_menu menu ) const {
    Coord pos;
    pos.y = menu_bar_rect().bottom_edge() + 8;
    if( g_menus[menu].right_side ) {
      pos.x = menu_header_x_pos( menu ) +
              menu_header_delta( menu ).w -
              menu_body_delta_inner( menu ).w;
    } else {
      pos.x = menu_header_x_pos( menu );
    }
    return Rect::from( pos, menu_body_delta_inner( menu ) );
  }

  Rect menu_body_rect( e_menu menu ) const {
    Coord pos;
    pos.y = menu_bar_rect().bottom_edge();
    if( g_menus[menu].right_side )
      pos.x = menu_header_x_pos( menu ) +
              menu_header_delta( menu ).w + 8 -
              menu_body_delta( menu ).w;
    else
      pos.x = menu_header_x_pos( menu ) - 8;
    return Rect::from( pos, menu_body_delta( menu ) );
  }

  // This includes (roughly) the space overwhich an open menu
  // occupies pixels (i.e., is not transparent). This is used to
  // decide if the user has clicked on or off of an open menu.
  Rect menu_body_clickable_area( e_menu menu ) const {
    auto res = menu_body_rect( menu );
    res.x += 8 / 2;
    res.w -= 8 / 2 * 2;
    res.h -= 8 / 2;
    return res;
  }

  // `h` is the vertical position from the top of the menu body.
  maybe<e_menu_item> cursor_to_item( e_menu menu, H h ) const {
    H pos{ 0 };
    for( auto const& item : g_menu_def[menu] ) {
      overload_visit(
          item,
          [&]( MenuItem::menu_divider const& ) {
            pos += divider_height();
          },
          [&]( MenuItem::menu_clickable const& ) {
            pos += menu_item_height();
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
  maybe<e_menu_item> cursor_to_item( e_menu menu,
                                     Coord  cursor ) const {
    if( !cursor.is_inside( menu_body_rect_inner( menu ) ) )
      return {};
    return cursor_to_item(
        menu,
        cursor.y - menu_body_rect_inner( menu ).top_edge() );
  }

  /****************************************************************
  ** Rendering Implmementation
  *****************************************************************/
  string_view name_for( e_menu menu ) const {
    return g_menus[menu].name;
  }

  string_view name_for( e_menu_item item ) const {
    return menu_items_[item]->name;
  }

  // For either a menu header or item.
  template<typename E>
  void render_menu_element( rr::Renderer& renderer, Coord pos,
                            E element, gfx::pixel color ) const {
    renderer.typer( "simple", pos, color )
        .write( name_for( element ) );
  }

  void render_divider( rr::Renderer& renderer, Coord pos,
                       e_menu menu ) const {
    Delta      delta      = divider_delta( menu );
    gfx::pixel color_fore = menu_theme_color2.shaded( 3 );
    pos.y += delta.h / 2;
    pos.x += 2;
    renderer.painter().draw_horizontal_line( pos, delta.w - 5,
                                             color_fore );
  }

  void render_item_background_selected( rr::Renderer& renderer,
                                        Coord         pos,
                                        e_menu menu ) const {
    gfx::rect background = gfx::rect{
        .origin = pos, .size = menu_item_delta( menu ) };
    renderer.painter().draw_solid_rect( background,
                                        menu_theme_color1 );
  }

  void render_menu_header_background( rr::Painter& painter,
                                      e_menu       menu,
                                      bool active ) const {
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
                         maybe<e_menu_item> subject ) const {
    rr::Painter painter = renderer.painter();
    if( subject.has_value() ) {
      CHECK( item_to_menu_[*subject] == menu );
    }

    render_rect_of_sprites_with_border(
        painter,                                           //
        pos,                                               //
        menu_body_delta( menu ) / Delta{ .w = 8, .h = 8 }, //
        e_tile::menu_body,                                 //
        e_tile::menu_top,                                  //
        e_tile::menu_bottom,                               //
        e_tile::menu_left,                                 //
        e_tile::menu_right,                                //
        e_tile::menu_top_left,                             //
        e_tile::menu_top_right,                            //
        e_tile::menu_bottom_left,                          //
        e_tile::menu_bottom_right                          //
    );

    pos.x += 8;
    pos.y += 8;

    H const item_height = menu_item_height();
    for( auto const& item : g_menu_def[menu] ) {
      overload_visit(
          item,
          [&]( MenuItem::menu_divider ) {
            render_divider( renderer, pos, menu );
            pos.y += divider_height();
          },
          [&]( MenuItem::menu_clickable const& clickable ) {
            bool on = ( clickable.item == subject );
            if( on )
              render_item_background_selected( renderer, pos,
                                               menu );
            gfx::pixel foreground_color =
                !is_menu_item_enabled( clickable.item )
                    ? foreground_disabled_color()
                : on ? menu_theme_color2
                     : menu_theme_color1;
            render_menu_element(
                renderer,
                pos + Delta{ .w = config_ui.menus.padding_x,
                             .h = ( ( item_height -
                                      max_text_height() ) /
                                    2 ) },
                clickable.item, foreground_color );
            pos.y += item_height;
          } );
    }
  }

  // Given `menu`, this matcher visits the global menu state
  // and renders the foreground/background for that menu. Use
  // a struct to visit so that we can get recursive
  // visitation.
  struct MenuRendererVisitor {
    // These would be the "lambda captures".
    Impl const*   impl;
    e_menu const& menu;
    Coord const&  background_upper_left;
    Coord const&  foreground_upper_left;
    rr::Renderer& renderer;

    void operator()( MenuState::menus_hidden ) const {}
    void operator()( MenuState::menus_closed closed ) const {
      rr::Painter painter = renderer.painter();
      if( menu == closed.hover )
        impl->render_menu_header_background( painter, menu,
                                             /*active=*/false );
      impl->render_menu_element( renderer, foreground_upper_left,
                                 menu, menu_theme_color1 );
    }
    void operator()( MenuState::item_click const& ic ) const {
      // Just forward this to the MenuState::menu_open.
      this->operator()(
          MenuState::menu_open{ impl->item_to_menu_[ic.item],
                                /*hover=*/{} } );
    }
    void operator()( MenuState::menu_open const& o ) const {
      if( o.menu != menu )
        return this->operator()( MenuState::menus_closed{} );
      rr::Painter painter = renderer.painter();
      impl->render_menu_header_background( painter, menu,
                                           /*active=*/true );
      impl->render_menu_element( renderer, foreground_upper_left,
                                 menu, menu_theme_color2 );
    }
  };

  void render_menu_bar( rr::Renderer& renderer ) const {
    // Render the "wood" panel. Start from the left edge of the
    // panel so that we get a continuous wood texture between the
    // two. Also, put the y position such that the menu bar gets
    // the bottom portion of the texture, again so that it will
    // be continuous with that panel.
    auto panel_upper_left =
        compositor::section( compositor::e_section::panel )
            .value_or( Rect{} )
            .upper_left();
    auto  bar_rect  = menu_bar_rect();
    Delta wood_size = sprite_size( e_tile::wood_middle );
    Coord start = panel_upper_left - Delta{ .h = wood_size.h };
    auto  wood_width    = wood_size.w;
    rr::Painter painter = renderer.painter();
    for( Coord c = start; c.x >= 0 - wood_width;
         c -= Delta{ .w = wood_width } )
      render_sprite( painter, e_tile::wood_middle, c );
    for( Coord c = start; c.x < bar_rect.right_edge();
         c += Delta{ .w = wood_width } )
      render_sprite( painter, e_tile::wood_middle, c );

    for( auto menu : refl::enum_values<e_menu> ) {
      auto  rect                  = menu_header_rect( menu );
      Coord background_upper_left = rect.upper_left();
      Coord foreground_upper_left =
          menu_header_text_rect( menu ).upper_left();
      MenuRendererVisitor matcher{
          this, menu, background_upper_left,
          foreground_upper_left, renderer };
      std::visit( matcher, menu_state_ );
    }
  }

  /****************************************************************
  ** Input Implementation
  *****************************************************************/
  // Use a struct to visit so that we can get recursive visita-
  // tion.
  struct ClickTargetVisitor {
    // These would be the "lambda captures".
    Impl&        impl;
    Coord const& screen_coord;

    using res_t = maybe<MouseOver_t>;

    res_t operator()( MenuState::menus_hidden ) const {
      return res_t{};
    }
    res_t operator()( MenuState::menus_closed ) const {
      for( auto menu : refl::enum_values<e_menu> )
        if( screen_coord.is_inside(
                impl.menu_header_rect( menu ) ) )
          return res_t{ MouseOver::header{ menu } };
      if( screen_coord.is_inside( impl.menu_bar_rect() ) )
        return res_t{ MouseOver::bar{} };
      return res_t( nothing );
    }
    res_t operator()( MenuState::item_click const& ic ) const {
      // Just forward this to the MenuState::menu_open.
      return ( *this )( MenuState::menu_open{
          impl.item_to_menu_[ic.item], /*hover=*/{} } );
    }
    res_t operator()( MenuState::menu_open const& o ) const {
      auto closed = ( *this )( MenuState::menus_closed{} );
      if( closed ) return res_t{ closed };
      if( !screen_coord.is_inside(
              impl.menu_body_clickable_area( o.menu ) ) )
        return res_t( nothing );
      // The cursor is over a non-transparent part of the open
      // menu.
      if( !screen_coord.is_inside(
              impl.menu_body_rect_inner( o.menu ) ) )
        return res_t{ MouseOver::border{ o.menu } };
      // The cursor is over the inner menu body, so at this
      // point its either over an item or a divider.
      auto maybe_item =
          impl.cursor_to_item( o.menu, screen_coord );
      if( !maybe_item.has_value() )
        return res_t{ MouseOver::divider{ o.menu } };
      // Finally, we are over an item.
      return res_t{ MouseOver::item{ *maybe_item } };
    }
  };

  maybe<MouseOver_t> click_target( Coord screen_coord ) {
    ClickTargetVisitor matcher{ *this, screen_coord };
    return std::visit( matcher, menu_state_ );
  }

  /****************************************************************
  ** Top-level Render Method
  *****************************************************************/
  void render_menus( rr::Renderer& renderer ) const {
    render_menu_bar( renderer );
    // maybe render open menu.
    overload_visit(
        menu_state_, []( MenuState::menus_hidden ) {},
        [&]( MenuState::menus_closed ) {},
        [&]( MenuState::item_click const& ic ) {
          auto  menu = item_to_menu_[ic.item];
          Coord pos  = menu_body_rect( menu ).upper_left();
          render_open_menu( renderer, pos, menu, ic.item );
        },
        [&]( MenuState::menu_open const& o ) {
          Coord pos = menu_body_rect( o.menu ).upper_left();
          render_open_menu( renderer, pos, o.menu, o.hover );
        } );
  }

  bool covers_screen() const override { return false; }

  void advance_state() override {
    if_get( menu_state_, MenuState::item_click, val ) {
      // We must cache this item before changing the menu state
      // since `val` will be invalidated once we do so.
      e_menu_item item = val.item;
      menu_state_      = MenuState::menus_closed{ /*hover=*/{} };
      DCHECK( menu_items_[item], "menu_items_[{}] is nullptr.",
              val.item );
      do_click( item );
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

  struct MouseMoveVisitor {
    Impl&           impl;
    e_input_handled operator()( MouseOver::bar ) {
      return e_input_handled::yes;
    }
    e_input_handled operator()( MouseOver::divider desc ) {
      CHECK( holds<MenuState::menu_open>( impl.menu_state_ ) ||
             holds<MenuState::item_click>( impl.menu_state_ ) );
      if( holds<MenuState::menu_open>( impl.menu_state_ ) ) {
        impl.menu_state_ =
            MenuState::menu_open{ desc.menu, /*hover=*/{} };
      }
      return e_input_handled::yes;
    }
    e_input_handled operator()( MouseOver::header header ) {
      if( holds<MenuState::menu_open>( impl.menu_state_ ) )
        impl.menu_state_ = MenuState::menu_open{ header.menu,
                                                 /*hover=*/{} };
      if( holds<MenuState::menus_closed>( impl.menu_state_ ) )
        impl.menu_state_ =
            MenuState::menus_closed{ /*hover=*/header.menu };
      return e_input_handled::yes;
    }
    e_input_handled operator()( MouseOver::item item ) {
      CHECK( holds<MenuState::menu_open>( impl.menu_state_ ) ||
             holds<MenuState::item_click>( impl.menu_state_ ) );
      if( holds<MenuState::menu_open>( impl.menu_state_ ) ) {
        auto& o =
            std::get<MenuState::menu_open>( impl.menu_state_ );
        CHECK( o.menu == impl.item_to_menu_[item.item] );
        o.hover = {};
        if( impl.is_menu_item_enabled( item.item ) )
          o.hover = item.item;
      }
      return e_input_handled::yes;
    }
    e_input_handled operator()( MouseOver::border border ) {
      // Delegate to the divider handler for now.
      return ( *this )( MouseOver::divider{ border.menu } );
    }
  };

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
          if( holds<MenuState::item_click>( menu_state_ ) )
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
              menu_state_ = MenuState::menus_closed{
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
            menu_state_ =
                MenuState::menus_closed{ /*hover=*/{} };
            return e_input_handled::yes;
          }
          if( menu.has_value() &&
              key_event.change == input::e_key_change::down &&
              is_alt ) {
            // Menus are open and the user is pressing an alt
            // key, so close menus.
            menu_state_ =
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
                  menu_state_ =
                      MenuState::menu_open{ menu, /*hover=*/{} };
                else
                  menu_state_ =
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
                if_get( menu_state_, MenuState::menu_open,
                        val ) {
                  if( val.hover.has_value() ) {
                    click_menu_item( val.hover.value() );
                    log_menu_state();
                  }
                }
                return e_input_handled::yes;
              case ::SDLK_ESCAPE:
                menu_state_ = MenuState::menus_closed{ {} };
                log_menu_state();
                return e_input_handled::yes;
              case ::SDLK_KP_4:
              case ::SDLK_LEFT: {
                menu = util::find_previous_and_cycle(
                    refl::enum_values<e_menu>, *menu );
                CHECK( menu );
                menu_state_ =
                    MenuState::menu_open{ *menu, /*hover=*/{} };
                log_menu_state();
                return e_input_handled::yes;
              }
              case ::SDLK_KP_6:
              case ::SDLK_RIGHT: {
                menu = util::find_subsequent_and_cycle(
                    refl::enum_values<e_menu>, *menu );
                CHECK( menu );
                menu_state_ =
                    MenuState::menu_open{ *menu, /*hover=*/{} };
                log_menu_state();
                return e_input_handled::yes;
              }
              case ::SDLK_KP_2:
              case ::SDLK_DOWN: {
                auto state = std::get<MenuState::menu_open>(
                    menu_state_ );
                if( !state.hover )
                  state.hover = items_from_menu_[*menu].back();
                auto start = state.hover;
                do {
                  state.hover = util::find_subsequent_and_cycle(
                      items_from_menu_[*menu], *state.hover );
                  if( state.hover == start )
                    // One or no menu items enabled.
                    break;
                } while( !is_menu_item_enabled( *state.hover ) );
                if( !is_menu_item_enabled( *state.hover ) )
                  // No menu items enabled.
                  return e_input_handled::yes;
                menu_state_ = state;
                log_menu_state();
                return e_input_handled::yes;
              }
              case ::SDLK_KP_8:
              case ::SDLK_UP: {
                auto state = std::get<MenuState::menu_open>(
                    menu_state_ );
                if( !state.hover )
                  state.hover = items_from_menu_[*menu].front();
                auto start = state.hover;
                do {
                  state.hover = util::find_previous_and_cycle(
                      items_from_menu_[*menu], *state.hover );
                  if( state.hover == start )
                    // One or no menu items enabled.
                    break;
                } while( !is_menu_item_enabled( *state.hover ) );
                if( !is_menu_item_enabled( *state.hover ) )
                  // No menu items enabled.
                  return e_input_handled::yes;
                menu_state_ = state;
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
        [this]( input::mouse_move_event_t mv_event ) {
          // Remove menu-hover by default and enable it again
          // below if the mouse if over a menu and menus are
          // closed.
          if( holds<MenuState::menus_closed>( menu_state_ ) )
            menu_state_ = MenuState::menus_closed{};
          auto over_what = click_target( mv_event.pos );
          if( !over_what.has_value() )
            return e_input_handled::no;
          MouseMoveVisitor matcher{ *this };
          return std::visit( matcher, *over_what );
        },
        [&, this]( input::mouse_button_event_t b_event ) {
          auto over_what = click_target( b_event.pos );
          if( !over_what.has_value() ) {
            if( holds<MenuState::menu_open>( menu_state_ ) ) {
              menu_state_ = MenuState::menus_closed{ {} };
              log_menu_state();
              return e_input_handled::yes; // no click through
            }
            return e_input_handled::no;
          }
          if( b_event.buttons ==
              input::e_mouse_button_event::left_down ) {
            return overload_visit(
                *over_what,
                [this]( MouseOver::bar ) {
                  menu_state_ = MenuState::menus_closed{ {} };
                  log_menu_state();
                  return e_input_handled::yes;
                },
                []( MouseOver::border ) {
                  return e_input_handled::yes;
                },
                []( MouseOver::divider ) {
                  return e_input_handled::yes;
                },
                [this]( MouseOver::header header ) {
                  if( !is_menu_open( header.menu ) )
                    menu_state_ =
                        MenuState::menu_open{ header.menu,
                                              /*hover=*/{} };
                  else
                    menu_state_ = MenuState::menus_closed{
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
                [this]( MouseOver::bar ) {
                  menu_state_ = MenuState::menus_closed{ {} };
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
        []( input::mouse_drag_event_t ) -> e_input_handled {
          // The framework does not send us mouse drag events
          // directly; instead it uses the api methods on the
          // Plane class.
          SHOULD_NOT_BE_HERE;
        } );
  }

  void click_menu_item( e_menu_item item ) {
    switch( menu_state_.to_enum() ) {
      case MenuState::e::item_click: {
        // Already clicking, so do nothing. This can happen if a
        // menu item is clicked after it is already in the click
        // animation.
        break;
      }
      case MenuState::e::menus_closed: {
        SHOULD_NOT_BE_HERE;
      }
      case MenuState::e::menus_hidden: {
        SHOULD_NOT_BE_HERE;
      }
      case MenuState::e::menu_open: {
        if( !is_menu_item_enabled( item ) ) return;
        lg.info( "selected menu item `{}`", item );
        menu_state_ = MenuState::item_click{
            item, chrono::system_clock::now() };
        log_menu_state();
        break;
      }
    };
  }

  void do_click( e_menu_item item ) const {
    CHECK( !handlers_[item].empty() );
    CHECK( handlers_[item].top() != nullptr );
    handlers_[item].top()->handle_menu_click( item );
  }

  bool is_menu_item_enabled( e_menu_item item ) const {
    if( handlers_[item].empty() ) return false;
    CHECK( handlers_[item].top() != nullptr );
    return handlers_[item].top()->will_handle_menu_click( item );
  }

  void register_handler( e_menu_item item, Plane& plane ) {
    handlers_[item].push( &plane );
  }

  void unregister_handler( e_menu_item item, Plane& plane ) {
    CHECK( !handlers_[item].empty() );
    CHECK( handlers_[item].top() == &plane );
    handlers_[item].pop();
  }
};

/****************************************************************
** Deregistrar
*****************************************************************/
void MenuPlane::Deregistrar::free_resource() {
  DCHECK( menu_plane_ != nullptr );
  DCHECK( plane_ != nullptr );
  menu_plane_->unregister_handler( resource(), *plane_ );
}

/****************************************************************
** MenuPlane
*****************************************************************/
Plane& MenuPlane::impl() { return *impl_; }

MenuPlane::~MenuPlane() = default;

MenuPlane::MenuPlane() : impl_( new Impl() ) {}

MenuPlane::Deregistrar MenuPlane::register_handler(
    e_menu_item item, Plane& plane ) {
  impl_->register_handler( item, plane );
  return Deregistrar{ *this, plane, item };
}

void MenuPlane::unregister_handler( e_menu_item item,
                                    Plane&      plane ) {
  impl_->unregister_handler( item, plane );
}

} // namespace rn
