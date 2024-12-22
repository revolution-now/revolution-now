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
#include "screen.hpp"
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
#include "gfx/coord.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::e_side;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

// TODO: move these to the configs with proper names after the
// old menu module is retired.
size constexpr kHighlightPadding{ .w = 6, .h = 2 };
size constexpr kBorderPadding{ .w = 4, .h = 4 };

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
** Rendered Layouts.
*****************************************************************/
MenuRenderLayout build_menu_rendered_layout(
    MenuContents const& contents,
    MenuAllowedPositions const& positions ) {
  CHECK( !positions.positions_allowed.empty() );
  MenuRenderLayout res;
  int y     = 0;
  int max_w = 0;
  int const row_height =
      rr::rendered_text_line_size_pixels( "X" ).h +
      2 * kHighlightPadding.h;
  auto add_item = [&]( string const& text ) -> auto& {
    auto& item = res.items.emplace_back();
    int const row_w =
        rr::rendered_text_line_size_pixels( text ).w +
        2 * kHighlightPadding.w;
    max_w = std::max( max_w, row_w );
    item  = {
       .text             = text,
       .bounds_relative  = { .origin = point{ .x = 0, .y = y },
                             .size   = { .h = row_height } },
       .text_nw_relative = point{} + kHighlightPadding };
    return item;
  };
  // Minus one to make it even.
  int const bar_height = row_height / 2 - 1;
  int needs_bar        = false;
  for( auto const& grp : contents.groups ) {
    if( needs_bar ) {
      res.bars.push_back( rect{ .origin = { .x = 0, .y = y },
                                .size = { .h = bar_height } } );
      y += bar_height;
    }
    needs_bar = true;
    for( auto const& elem : grp.elems ) {
      SWITCH( elem ) {
        CASE( leaf ) {
          auto& item =
              add_item( config_menu.items[leaf.item].name );
          item.has_arrow = false;
          break;
        }
        CASE( node ) {
          // Ensure there is some space reserved for the arrow
          // which will be rendered on the right hand side.
          auto& item     = add_item( node.text + "  " );
          item.has_arrow = true;
          break;
        }
      }
      y += row_height;
    }
  }

  // Now apply border padding.
  for( auto& layout : res.items )
    layout.bounds_relative.origin += kBorderPadding;
  for( rect& bar : res.bars ) bar.origin.y += kBorderPadding.h;
  size const body_size =
      size{ .w = max_w, .h = y } + kBorderPadding * 2;

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
    for( auto const& position_allowed :
         positions.positions_allowed ) {
      auto orientations = position_allowed.orientations_allowed;
      if( orientations.empty() )
        // Note that we could make use of the parent_side prop-
        // erty to make a more informed guess here about which
        // order to choose, but it's not expected that orienta-
        // tions won't be provided for submenus.
        orientations = {
          e_diagonal_direction::nw, e_diagonal_direction::sw,
          e_diagonal_direction::ne, e_diagonal_direction::se };
      for( e_diagonal_direction const orientation :
           orientations ) {
        MenuPosition const position{
          .where       = position_allowed.where,
          .orientation = orientation,
          .parent_side = position_allowed.parent_side };
        res.push_back( PositionAndBounds{
          .position = position,
          .bounds =
              compute_bounding_rect( position, body_size ) } );
      }
    }
    return res;
  }();
  CHECK( !possible_positions.empty() );

  rect const screen_rect    = main_window_logical_rect();
  auto const fitting_bounds = [&] {
    auto fitting = possible_positions;
    erase_if(
        fitting, [&]( PositionAndBounds const& bounds_rect ) {
          return !bounds_rect.bounds.is_inside( screen_rect );
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
** Menu Rendering.
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

void render_menu_body( rr::Renderer& renderer,
                       MenuAnimState const& anim_state,
                       MenuRenderLayout const& layout ) {
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

  static auto const text_color = config_ui.dialog_text.normal;

  for( auto const& item_layout : layout.items ) {
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
    if( anim_state.highlighted == item_layout.text ) {
      painter.draw_solid_rect(
          item_layout.bounds_absolute,
          config_ui.dialog_text.selected_background );
      rr::Typer typer =
          renderer.typer( item_layout.bounds_absolute.origin +
                              item_layout.text_nw_relative
                                  .distance_from_origin(),
                          text_color );
      typer.write( item_layout.text );
    } else {
      rr::Typer typer =
          renderer.typer( item_layout.bounds_absolute.origin +
                              item_layout.text_nw_relative
                                  .distance_from_origin(),
                          text_color );
      typer.write( item_layout.text );
    }
    if( item_layout.has_arrow )
      render_sprite_silhouette( renderer, arrow_origin,
                                e_tile::submenu_arrow,
                                text_color );
  }

  for( auto const& bar : layout.bars ) {
    auto const r = bar;
    point const start{ .x = r.left(),
                       .y = ( r.bottom() + r.top() ) / 2 };
    render_divider( renderer, start, r.size.w );
  }
}

} // namespace rn
