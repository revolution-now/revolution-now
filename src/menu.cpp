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
#include "compositor.hpp"
#include "error.hpp"
#include "frame.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "plane.hpp"
#include "render.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "throttler.hpp"
#include "tiles.hpp"
#include "variant.hpp"

// Rds
#include "menu-impl.rds.hpp"

// config
#include "config/menu.rds.hpp"
#include "config/tile-enum.rds.hpp"
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

using config::menu::e_menu_side;
using config::menu::MenuItemConfig;

} // namespace

/****************************************************************
** MenuPlane::Impl
*****************************************************************/
struct MenuPlane::Impl : public Plane {
  // State
  refl::enum_map<e_menu, int>      menu_name_width_pixels_;
  refl::enum_map<e_menu_item, int> menu_item_name_width_pixels_;

  refl::enum_map<e_menu_item, e_menu>         item_to_menu_;
  refl::enum_map<e_menu, vector<e_menu_item>> items_from_menu_;

  MenuState menu_state_{ MenuState::menus_closed{} };

  refl::enum_map<e_menu_item, stack<Plane*>> handlers_;

  Impl() {
    auto& menus_layout = config_menu.layout.menus;

    // Populate the e_menu_item maps.
    for( auto& [menu, items] : menus_layout ) {
      for( auto& item_desc : items.contents ) {
        if( !item_desc.has_value() ) continue;
        e_menu_item const item = *item_desc;
        items_from_menu_[menu].push_back( item );
        item_to_menu_[item] = menu;
      }
    }

    // Populate text widths of menu and menu item names.
    int kCharWidth = rr::rendered_text_line_size_pixels( "X" ).w;
    for( auto menu : refl::enum_values<e_menu> )
      menu_name_width_pixels_[menu] =
          menus_layout[menu].name.size() * kCharWidth;
    for( auto item : refl::enum_values<e_menu_item> )
      menu_item_name_width_pixels_[item] =
          config_menu.items[item].name.size() * kCharWidth;
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
  static gfx::pixel const menu_theme_color1() {
    return config_ui.dialog_text.normal;
  }
  static gfx::pixel const menu_theme_color2() {
    return gfx::pixel::wood();
  }

  auto foreground_disabled_color() const {
    return config_ui.dialog_text.disabled;
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
    auto const& menus = config_menu.layout.menus;
    auto const& desc  = menus[target];
    W           width_delta{ 0 };
    if( desc.position == e_menu_side::right ) {
      width_delta = rl::rall( refl::enum_values<e_menu> )
                        .remove_if_L( menus[_].position !=
                                      e_menu_side::right )
                        .take_while_incl_L( _ != target )
                        .map_L( menu_header_delta( _ ).w )
                        .intersperse( config_ui.menus.spacing_x )
                        .accumulate();
    } else {
      width_delta = rl::all( refl::enum_values<e_menu> )
                        .remove_if_L( menus[_].position ==
                                      e_menu_side::right )
                        .take_while_L( _ != target )
                        .map_L( menu_header_delta( _ ).w +
                                config_ui.menus.spacing_x )
                        .accumulate();
    }
    width_delta += config_ui.menus.first_menu_start_x_offset;
    CHECK( width_delta >= 0 );
    Rect rect = menu_bar_rect();
    return rect.x + ( ( desc.position != e_menu_side::right )
                          ? width_delta
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
    // round up to nearest multiple of 4, since that is the menu
    // tile width.
    if( res % 4 != 0 ) res += ( 4 - ( res % 4 ) );
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
    auto const& menus = config_menu.layout.menus;
    H           h{ 0 };
    for( maybe<e_menu_item> const& item :
         menus[menu].contents ) {
      if( item.has_value() )
        h += menu_item_height();
      else // divider.
        h += divider_height();
    }
    // round up to nearest multiple of 4, since that is the menu
    // "tile" width.
    if( h % 4 != 0 ) h += ( 4 - ( h % 4 ) );
    return h;
  }

  Delta menu_body_delta_inner( e_menu menu ) const {
    return { menu_body_width_inner( menu ),
             menu_body_height_inner( menu ) };
  }

  Delta menu_body_delta( e_menu menu ) const {
    return Delta{ .w = 4, .h = 4 } + Delta{ .w = 4, .h = 4 } +
           menu_body_delta_inner( menu );
  }

  Rect menu_body_rect_inner( e_menu menu ) const {
    auto const& menus = config_menu.layout.menus;
    Coord       pos;
    pos.y = menu_bar_rect().bottom_edge() + 4;
    if( menus[menu].position == e_menu_side::right ) {
      pos.x = menu_header_x_pos( menu ) +
              menu_header_delta( menu ).w -
              menu_body_delta_inner( menu ).w;
    } else {
      pos.x = menu_header_x_pos( menu );
    }
    return Rect::from( pos, menu_body_delta_inner( menu ) );
  }

  Rect menu_body_rect( e_menu menu ) const {
    auto const& menus = config_menu.layout.menus;
    Coord       pos;
    pos.y = menu_bar_rect().bottom_edge();
    if( menus[menu].position == e_menu_side::right )
      pos.x = menu_header_x_pos( menu ) +
              menu_header_delta( menu ).w + 4 -
              menu_body_delta( menu ).w;
    else
      pos.x = menu_header_x_pos( menu ) - 4;
    return Rect::from( pos, menu_body_delta( menu ) );
  }

  // This includes (roughly) the space overwhich an open menu
  // occupies pixels (i.e., is not transparent). This is used to
  // decide if the user has clicked on or off of an open menu.
  Rect menu_body_clickable_area( e_menu menu ) const {
    auto res = menu_body_rect( menu );
    res.x += 4 / 2;
    res.w -= 4 / 2 * 2;
    res.h -= 4 / 2;
    return res;
  }

  // `h` is the vertical position from the top of the menu body.
  maybe<e_menu_item> cursor_to_item( e_menu menu, H h ) const {
    auto const& menus = config_menu.layout.menus;
    H           pos{ 0 };
    for( maybe<e_menu_item> const& item :
         menus[menu].contents ) {
      if( item.has_value() )
        pos += menu_item_height();
      else // divider.
        pos += divider_height();
      if( pos > h ) {
        if( item.has_value() )
          return item;
        else // divider.
          return nothing;
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
    auto const& menus = config_menu.layout.menus;
    return menus[menu].name;
  }

  string_view name_for( e_menu_item item ) const {
    return config_menu.items[item].name;
  }

  // For either a menu header or item.
  template<typename E>
  void render_menu_element( rr::Renderer& renderer, Coord where,
                            E element, gfx::pixel color,
                            maybe<char> shortcut ) const {
    string name = string( name_for( element ) );
    if( shortcut ) {
      // Highlight the letter representing the shortcut key.
      string const shortcut_str( 1, *shortcut );
      auto         pos = name.find_first_of( shortcut_str );
      if( pos != string::npos )
        name.replace( pos, 1,
                      fmt::format( "[{}]", shortcut_str ) );
    }
    render_text_markup( renderer, where, /*font=*/{},
                        TextMarkupInfo{ .normal = color },
                        name );
  }

  void render_menu_element( rr::Renderer& renderer, Coord pos,
                            e_menu     item,
                            gfx::pixel color ) const {
    auto const& menus = config_menu.layout.menus;
    render_menu_element( renderer, pos, item, color,
                         menus[item].shortcut );
  }

  void render_menu_element( rr::Renderer& renderer, Coord pos,
                            e_menu_item item,
                            gfx::pixel  color ) const {
    render_menu_element( renderer, pos, item, color,
                         /*shortcut=*/nothing );
  }

  void render_divider( rr::Renderer& renderer, Coord pos,
                       e_menu menu ) const {
    Delta const delta = divider_delta( menu );
    pos.y += delta.h / 2;
    pos.x -= 4;
    renderer.painter().draw_horizontal_line(
        pos + Delta{ .w = 1, .h = -1 }, delta.w + 2,
        config_ui.window.border_dark );
    renderer.painter().draw_horizontal_line(
        pos + Delta{ .w = ( delta.w * 3 ) / 4 + 7, .h = -1 },
        delta.w / 3 - 12, config_ui.window.border_light );
    renderer.painter().draw_horizontal_line(
        pos, delta.w, config_ui.window.border_darker );
    renderer.painter().draw_horizontal_line(
        pos + Delta{ .w = ( delta.w * 3 ) / 4 + 4 },
        delta.w / 3 - 9, config_ui.window.border_dark );
    renderer.painter().draw_horizontal_line(
        pos + Delta{ .w = 1, .h = +1 }, delta.w + 3,
        config_ui.window.border_light );
    renderer.painter().draw_horizontal_line(
        pos + Delta{ .w = ( delta.w * 3 ) / 4, .h = +1 },
        delta.w / 3 - 5, config_ui.window.border_lighter );
  }

  void render_item_background_selected( rr::Renderer& renderer,
                                        Coord         pos,
                                        e_menu menu ) const {
    gfx::rect background = gfx::rect{
        .origin = pos, .size = menu_item_delta( menu ) };
    renderer.painter().draw_solid_rect(
        background, config_ui.dialog_text.selected_background );
  }

  void render_menu_header_background( rr::Renderer& renderer,
                                      e_menu        menu,
                                      bool active ) const {
    Rect const header = menu_header_rect( menu );
    if( active ) {
      rr::Painter painter = renderer.painter();
      painter.draw_solid_rect(
          header, config_ui.dialog_text.selected_background );
    } else {
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .5 );
      rr::Painter painter = renderer.painter();
      painter.draw_solid_rect(
          header, config_ui.dialog_text.selected_background );
    }
  }

  void render_open_menu( rr::Renderer& renderer, Coord pos,
                         e_menu             menu,
                         maybe<e_menu_item> subject ) const {
    rr::Painter painter = renderer.painter();
    if( subject.has_value() ) {
      CHECK( item_to_menu_[*subject] == menu );
    }

    Rect const body_rect =
        Rect::from( pos, menu_body_delta( menu ) );
    tile_sprite( painter, e_tile::wood_middle, body_rect );
    render_shadow_hightlight_border(
        renderer, body_rect.edges_removed( 2 ),
        config_ui.window.border_dark,
        config_ui.window.border_lighter );
    render_shadow_hightlight_border(
        renderer, body_rect.edges_removed( 1 ),
        config_ui.window.border_darker,
        config_ui.window.border_light );

    pos.x += 4;
    pos.y += 4;

    H const     item_height = menu_item_height();
    auto const& menus       = config_menu.layout.menus;
    for( maybe<e_menu_item> const& item :
         menus[menu].contents ) {
      if( item.has_value() ) {
        bool on = ( *item == subject );
        if( on )
          render_item_background_selected( renderer, pos, menu );
        gfx::pixel foreground_color =
            !is_menu_item_enabled( *item )
                ? foreground_disabled_color()
                : menu_theme_color1();
        render_menu_element(
            renderer,
            pos + Delta{ .w = config_ui.menus.padding_x,
                         .h = ( ( item_height -
                                  max_text_height() ) /
                                2 ) },
            *item, foreground_color );
        pos.y += item_height;
      } else { // divider.
        render_divider( renderer, pos, menu );
        pos.y += divider_height();
      }
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
      if( menu == closed.hover )
        impl->render_menu_header_background( renderer, menu,
                                             /*active=*/false );
      impl->render_menu_element( renderer, foreground_upper_left,
                                 menu, menu_theme_color1() );
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
      impl->render_menu_header_background( renderer, menu,
                                           /*active=*/true );
      impl->render_menu_element( renderer, foreground_upper_left,
                                 menu, menu_theme_color1() );
    }
  };

  void render_menu_bar( rr::Renderer& renderer ) const {
    // Render the "wood" panel. Start from the left edge of the
    // panel so that we get a continuous wood texture between the
    // two. Also, put the y position such that the menu bar gets
    // the bottom portion of the texture, again so that it will
    // be continuous with that panel.
    Rect const panel_rect =
        compositor::section( compositor::e_section::panel )
            .value_or( Rect{} );
    Coord const panel_upper_left = panel_rect.upper_left();
    Rect const  bar_rect         = menu_bar_rect();
    Delta const wood_size = sprite_size( e_tile::wood_middle );
    Coord const start =
        panel_upper_left - Delta{ .h = wood_size.h };
    W const     wood_width = wood_size.w;
    rr::Painter painter    = renderer.painter();
    for( Coord c = start; c.x >= 0 - wood_width;
         c -= Delta{ .w = wood_width } )
      render_sprite( painter, e_tile::wood_middle, c );
    for( Coord c = start; c.x < bar_rect.right_edge();
         c += Delta{ .w = wood_width } )
      render_sprite( painter, e_tile::wood_middle, c );

    // Render some border lines.
    painter.draw_horizontal_line(
        bar_rect.lower_left() + Delta{ .h = -2 },
        bar_rect.delta().w - panel_rect.delta().w + 1,
        config_ui.window.border_dark );
    painter.draw_horizontal_line(
        bar_rect.lower_left() + Delta{ .h = -1 },
        bar_rect.delta().w - panel_rect.delta().w,
        config_ui.window.border_darker );

    for( auto menu : refl::enum_values<e_menu> ) {
      auto  rect                  = menu_header_rect( menu );
      Coord background_upper_left = rect.upper_left();
      Coord foreground_upper_left =
          menu_header_text_rect( menu ).upper_left();
      MenuRendererVisitor matcher{
          this, menu, background_upper_left,
          foreground_upper_left, renderer };
      base::visit( matcher, menu_state_.as_base() );
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

    using res_t = maybe<MouseOver>;

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

  maybe<MouseOver> click_target( Coord screen_coord ) {
    ClickTargetVisitor matcher{ *this, screen_coord };
    return base::visit( matcher, menu_state_.as_base() );
  }

  /****************************************************************
  ** Top-level Render Method
  *****************************************************************/
  void render_menus( rr::Renderer& renderer ) const {
    render_menu_bar( renderer );
    // maybe render open menu.
    overload_visit(
        menu_state_.as_base(), []( MenuState::menus_hidden ) {},
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
              auto const& menus     = config_menu.layout.menus;
              auto const& menu_desc = menus[menu];
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
              default:
                break;
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
          return base::visit( matcher, over_what->as_base() );
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
