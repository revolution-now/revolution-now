/****************************************************************
**menu-render.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-15.
*
* Description: Renders menus.
*
*****************************************************************/
#include "menu-render.hpp"

// Revolution Now
#include "render.hpp"
#include "text.hpp"
#include "tiles.hpp"

// config
#include "config/menu-items.rds.hpp"
#include "config/menu.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"

// render
#include "error.hpp"
#include "render/extra.hpp"
#include "render/painter.hpp"
#include "render/renderer.hpp"
#include "render/typer.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/coord.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// C++ standard library
#include <ranges>

namespace views = std::ranges::views;

using namespace std;

namespace rn {

namespace {

using ::base::function_ref;
using ::gfx::e_side;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;
using ::refl::enum_count;

gfx::rect compute_bounding_rect( MenuPosition const& position,
                                 size const sz ) {
  point const p = position.where;
  point nw;
  size delta =
      p.moved( reverse_direction( position.orientation ) ) - p;
  delta.w *= sz.w;
  delta.h *= sz.h;
  switch( position.orientation ) {
    case e_diagonal_direction::se: {
      nw = p + delta;
      break;
    }
    case e_diagonal_direction::ne: {
      delta.h = 0;
      nw      = p + delta;
      break;
    }
    case e_diagonal_direction::sw: {
      delta.w = 0;
      nw      = p + delta;
      break;
    }
    case e_diagonal_direction::nw:
      delta.w = 0;
      delta.h = 0;
      nw      = p + delta;
      break;
  }
  return rect{ .origin = nw, .size = sz }.normalized();
}

} // namespace

/****************************************************************
** MenuBarAnimState
*****************************************************************/
maybe<e_menu> MenuBarAnimState::highlighted_menu() const {
  if( !focused.has_value() ) return nothing;
  if( highlight_state != e_menu_highlight_state::highlighted )
    return nothing;
  return *focused;
}

maybe<e_menu> MenuBarAnimState::opened_menu() const {
  if( !focused.has_value() ) return nothing;
  if( highlight_state != e_menu_highlight_state::open )
    return nothing;
  return *focused;
}

void MenuBarAnimState::set_highlighted( e_menu menu ) {
  focused         = menu;
  highlight_state = e_menu_highlight_state::highlighted;
}

void MenuBarAnimState::set_opened( e_menu menu ) {
  focused         = menu;
  highlight_state = e_menu_highlight_state::open;
}

void MenuBarAnimState::clear_focus() { *this = {}; }

/****************************************************************
** Menu Body Rendered Layouts.
*****************************************************************/
MenuRenderLayout build_menu_rendered_layout(
    e_menu const menu, rect const logical_screen_rect,
    MenuAllowedPositions const& positions ) {
  auto const& contents = config_menu.layout[menu];
  CHECK( !positions.positions_allowed.empty() );
  MenuRenderLayout res;
  int y     = 0;
  int max_w = 0;
  int const row_height =
      rr::rendered_text_line_size_pixels( "X" ).h +
      2 * config_ui.menus.highlight_padding.h;
  auto add_item = [&]( string const& text ) -> auto& {
    auto& item = res.items.emplace_back();
    int const row_w =
        rr::rendered_text_line_size_pixels( text ).w +
        2 * config_ui.menus.highlight_padding.w;
    max_w = std::max( max_w, row_w );
    item  = {
       .text            = text,
       .bounds_relative = { .origin = point{ .x = 0, .y = y },
                            .size   = { .h = row_height } },
       .text_nw_relative =
          point{} + config_ui.menus.highlight_padding };
    return item;
  };
  // Minus one to make it even.
  int const bar_height = row_height / 2 - 1;
  for( auto const& elem : contents.contents ) {
    if( !elem.has_value() ) {
      res.bars.push_back( rect{ .origin = { .x = 0, .y = y },
                                .size = { .h = bar_height } } );
      y += bar_height;
      continue;
    }
    SWITCH( *elem ) {
      CASE( leaf ) {
        auto& item =
            add_item( config_menu.items[leaf.item].name );
        item.item      = leaf.item;
        item.has_arrow = false;
        break;
      }
      CASE( node ) {
        // Ensure there is some space reserved for the arrow
        // which will be rendered on the right hand side.
        auto& item =
            add_item( config_menu.menus[node.menu].name + "  " );
        item.has_arrow = true;
        break;
      }
    }
    y += row_height;
  }

  // Now apply border padding.
  for( auto& layout : res.items )
    layout.bounds_relative.origin +=
        config_ui.menus.body_border_padding;
  for( rect& bar : res.bars )
    bar.origin.y += config_ui.menus.body_border_padding.h;
  size const body_size = size{ .w = max_w, .h = y } +
                         config_ui.menus.body_border_padding * 2;

  // Now make the widths uniform.
  for( auto& rendered_item_layout : res.items )
    rendered_item_layout.bounds_relative.size.w = max_w;
  for( rect& bar : res.bars ) bar.size.w = body_size.w;

  // Now populate the absolute coordinates.
  struct PositionAndBounds {
    MenuPosition position;
    rect bounds;
  };

  vector<PositionAndBounds> const possible_positions = [&] {
    vector<PositionAndBounds> res;
    // 4 = # diagonal directions.
    res.reserve( positions.positions_allowed.size() * 4 );
    for( auto const& allowed : positions.positions_allowed ) {
      MenuPosition const position{
        .where       = allowed.where,
        .orientation = allowed.orientation,
        .parent_side = allowed.parent_side };
      res.push_back( PositionAndBounds{
        .position = position,
        .bounds =
            compute_bounding_rect( position, body_size ) } );
    }
    return res;
  }();
  CHECK( !possible_positions.empty() );

  auto const fitting_bounds = [&] {
    auto fitting = possible_positions;
    erase_if( fitting,
              [&]( PositionAndBounds const& bounds_rect ) {
                return !bounds_rect.bounds.is_inside(
                    logical_screen_rect );
              } );
    return fitting;
  }();

  PositionAndBounds const& chosen_position =
      !fitting_bounds.empty() ? fitting_bounds[0]
                              : possible_positions[0];
  res.position = chosen_position.position;
  res.bounds   = chosen_position.bounds;
  for( auto& rendered_item_layout : res.items ) {
    rendered_item_layout.bounds_absolute.size =
        rendered_item_layout.bounds_relative.size;
    rendered_item_layout.bounds_absolute.origin =
        rendered_item_layout.bounds_relative.origin
            .origin_becomes_point( res.bounds.origin );
  }
  for( rect& bar : res.bars )
    bar.origin =
        bar.origin.origin_becomes_point( res.bounds.origin );
  return res;
}

/****************************************************************
** Menu Bar Rendered Layouts.
*****************************************************************/
MenuBarRenderedLayout build_menu_bar_rendered_layout(
    rect const logical_screen_rect,
    vector<e_menu> const& contents ) {
  MenuBarRenderedLayout res;
  auto const menu_bar_rect =
      logical_screen_rect.with_new_bottom_edge(
          config_ui.menus.menu_bar_height );
  res.bounds = menu_bar_rect;

  auto menus_for = [&]( config::menu::e_menu_side const side ) {
    vector<e_menu> res;
    res.reserve( enum_count<e_menu> );
    for( e_menu const menu : contents )
      if( config_menu.menus[menu].position == side )
        res.push_back( menu );
    return res;
  };

  vector<e_menu> const l_menus =
      menus_for( config::menu::e_menu_side::left );
  vector<e_menu> const r_menus =
      menus_for( config::menu::e_menu_side::right );

  point p;

  auto add_header = [&]( e_menu const menu, bool const x_pre ) {
    MenuHeaderRenderLayout header;
    header.menu = menu;
    header.text = config_menu.menus[menu].name;
    size const text_size =
        rr::rendered_text_line_size_pixels( header.text );
    size const header_size =
        text_size + config_ui.menus.highlight_padding * 2;
    p.y = res.bounds.origin.y;
    p.y += ( res.bounds.size.h - header_size.h ) / 2;
    --p.y;
    if( x_pre ) p.x -= header_size.w;
    header.bounds_absolute = { .origin = p,
                               .size   = header_size };
    if( !x_pre ) p.x += header_size.w;
    header.text_nw_absolute =
        gfx::centered_in( text_size, header.bounds_absolute );
    res.headers.push_back( header );
  };

  p = res.bounds.nw();
  p.x += config_ui.menus.menu_bar_x_padding;
  for( e_menu const menu : l_menus )
    add_header( menu, /*x_pre=*/false );

  p = res.bounds.ne();
  p.x -= config_ui.menus.menu_bar_x_padding;
  for( e_menu const menu : views::reverse( r_menus ) )
    add_header( menu, /*x_pre=*/true );
  return res;
}

/****************************************************************
** Menu Body Rendering.
*****************************************************************/
static void render_divider( rr::Renderer& renderer,
                            point const pos,
                            int const max_width ) {
  int const width     = max_width - 8;
  rr::Painter painter = renderer.painter();
  painter.draw_horizontal_line( pos + size{ .w = 1, .h = -1 },
                                width + 2,
                                config_ui.window.border_dark );
  painter.draw_horizontal_line(
      pos + size{ .w = ( width * 3 ) / 4 + 7, .h = -1 },
      width / 3 - 12, config_ui.window.border_light );
  painter.draw_horizontal_line( pos, width,
                                config_ui.window.border_darker );
  painter.draw_horizontal_line(
      pos + size{ .w = ( width * 3 ) / 4 + 4 }, width / 3 - 9,
      config_ui.window.border_dark );
  painter.draw_horizontal_line( pos + size{ .w = 1, .h = +1 },
                                width + 3,
                                config_ui.window.border_light );
  painter.draw_horizontal_line(
      pos + size{ .w = ( width * 3 ) / 4, .h = +1 },
      width / 3 - 5, config_ui.window.border_lighter );
}

void render_menu_body(
    rr::Renderer& renderer, MenuAnimState const& anim_state,
    MenuRenderLayout const& layout,
    function_ref<ItemEnabledFn> const enabled_fn ) {
  SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, anim_state.alpha )
  rect const body = layout.bounds;

  tile_sprite( renderer, e_tile::wood_middle, body );
  render_shadow_hightlight_border(
      renderer, body.with_edges_removed( 2 ),
      config_ui.window.border_dark,
      config_ui.window.border_lighter );
  render_shadow_hightlight_border(
      renderer, body.with_edges_removed( 1 ),
      config_ui.window.border_darker,
      config_ui.window.border_light );

  rr::Painter painter = renderer.painter();

  for( auto const& item_layout : layout.items ) {
    // Highlight.
    if( anim_state.highlighted == item_layout.text )
      painter.draw_solid_rect(
          item_layout.bounds_absolute,
          config_ui.dialog_text.selected_background );

    // Text.
    auto const text_color = [&] {
      if( item_layout.item.has_value() )
        return enabled_fn( item_layout )
                   ? config_ui.dialog_text.normal
                   : config_ui.dialog_text.disabled;
      else
        return config_ui.dialog_text.normal;
    }();
    rr::Typer typer = renderer.typer(
        item_layout.bounds_absolute.origin +
            item_layout.text_nw_relative.distance_from_origin(),
        text_color );
    typer.write( item_layout.text );

    // Arrow.
    if( item_layout.has_arrow ) {
      auto const arrow_origin = [&] {
        point p = item_layout.bounds_absolute.center();
        p.x     = item_layout.bounds_absolute.right();
        static auto const arrow_size =
            sprite_size( e_tile::submenu_arrow );
        p = centered_on( arrow_size, p ).nw();
        // Looks better with a bit of space on its right.
        p.x -= 1;
        return p;
      }();
      render_sprite_silhouette( renderer, arrow_origin,
                                e_tile::submenu_arrow,
                                text_color );
    }
  }

  for( auto const& bar : layout.bars ) {
    auto const r = bar;
    point const start{ .x = r.left(),
                       .y = ( r.bottom() + r.top() ) / 2 };
    render_divider( renderer, start, r.size.w );
  }
}

/****************************************************************
** Menu Bar Rendering.
*****************************************************************/
static void render_wood_bar(
    rr::Renderer& renderer,
    MenuBarRenderedLayout const& layout ) {
  tile_sprite( renderer, e_tile::wood_middle, layout.bounds );

  rr::Painter painter = renderer.painter();

  int const panel_width = config_ui.panel.width;

  // Render some border lines.
  //
  // The lengths of these lines (as well as the point pixel) are
  // chosen to make a connection with the corresponding border
  // that will be drawn by the panel plane so that we form a con-
  // tiguous corner that makes the menu bar and panel look like a
  // single wood slab.
  painter.draw_horizontal_line(
      layout.bounds.sw() + size{ .h = -2 },
      layout.bounds.size.w - panel_width + 2,
      config_ui.window.border_dark );
  painter.draw_point(
      layout.bounds.se() - size{ .w = panel_width - 1, .h = 1 },
      config_ui.window.border_dark );
  painter.draw_horizontal_line(
      layout.bounds.sw() + size{ .h = -1 },
      layout.bounds.size.w - panel_width + 1,
      config_ui.window.border_darker );
}

static void render_header_text( rr::Renderer& renderer,
                                point const where,
                                e_menu const menu,
                                pixel const color ) {
  auto const& shortcut = config_menu.menus[menu].shortcut;
  string const name    = [&] {
    string res = config_menu.menus[menu].name;
    if( !shortcut.has_value() ) return res;
    // Highlight the letter representing the shortcut key.
    string const shortcut_str( 1, *shortcut );
    auto const pos = name.find_first_of( shortcut_str );
    if( pos != string::npos )
      res.replace( pos, 1, fmt::format( "[{}]", shortcut_str ) );
    return res;
  }();
  render_text_markup( renderer, where, /*font=*/{},
                      TextMarkupInfo{ .normal = color }, name );
}

void render_menu_bar( rr::Renderer& renderer,
                      MenuBarAnimState const& anim_state,
                      MenuBarRenderedLayout const& layout ) {
  render_wood_bar( renderer, layout );

  rr::Painter painter = renderer.painter();
  for( auto const& header : layout.headers ) {
    if( anim_state.focused == header.menu ) {
      switch( anim_state.highlight_state ) {
        case rn::e_menu_highlight_state::highlighted: {
          SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .5 );
          rr::Painter painter = renderer.painter();
          painter.draw_solid_rect(
              header.bounds_absolute,
              config_ui.dialog_text.selected_background );
          break;
        }
        case rn::e_menu_highlight_state::open: {
          painter.draw_solid_rect(
              header.bounds_absolute,
              config_ui.dialog_text.selected_background );
          break;
        }
      }
    }
    render_header_text( renderer, header.text_nw_absolute,
                        header.menu,
                        config_ui.dialog_text.normal );
  }
}

} // namespace rn
