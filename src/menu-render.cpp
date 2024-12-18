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

// config
#include "config/menu-items.rds.hpp"

// render
#include "error.hpp"
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

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

gfx::rect compute_bounding_rect( MenuPosition const& position,
                                 size const sz ) {
  point const p = position.where;
  point nw;
  size delta =
      p.moved( reverse_direction( position.corner ) ) - p;
  delta.w *= sz.w;
  delta.h *= sz.h;
  switch( position.corner ) {
    case e_direction::se: {
      nw = p + delta;
      break;
    }
    case e_direction::ne: {
      delta.h = 0;
      nw      = p + delta;
      break;
    }
    case e_direction::sw: {
      delta.w = 0;
      nw      = p + delta;
      break;
    }
    case e_direction::nw:
      delta.w = 0;
      delta.h = 0;
      nw      = p + delta;
      break;
    default:
      FATAL( "direction {} not supported here.",
             position.corner );
  }
  return rect{ .origin = nw, .size = sz }.normalized();
}

} // namespace

/****************************************************************
** Rendered Layouts.
*****************************************************************/
MenuRenderLayout build_menu_rendered_layout(
    MenuContents const& contents,
    MenuPosition const& position ) {
  MenuRenderLayout res;
  int y         = 0;
  int max_w     = 0;
  auto add_item = [&]( string const& text ) -> auto& {
    auto& item = res.items.emplace_back();
    size const text_size =
        rr::rendered_text_line_size_pixels( text );
    max_w = std::max( max_w, text_size.w );
    item  = {
       .text            = text,
       .bounds_relative = { .origin = { .x = 0, .y = y },
                            .size   = { .h = text_size.h } } };
    return item;
  };
  int const text_height =
      rr::rendered_text_line_size_pixels( "X" ).h;
  int const bar_height = text_height - 4;
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
          auto& item = add_item( fmt::to_string( leaf.item ) );
          item.has_arrow = false;
          break;
        }
        CASE( node ) {
          auto& item     = add_item( node.text + " >>" );
          item.has_arrow = true;
          break;
        }
      }
      y += text_height;
    }
  }
  size const body_size{ .w = max_w, .h = y };
  for( auto& rendered_item_layout : res.items )
    rendered_item_layout.bounds_relative.size.w = max_w;
  for( rect& bar : res.bars ) bar.size.w = max_w;
  res.bounds = compute_bounding_rect( position, body_size );
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
** MenuRenderer
*****************************************************************/
void render_menu_body( rr::Renderer& renderer,
                       MenuAnimState const& anim_state,
                       MenuRenderLayout const& layout ) {
  SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, anim_state.alpha )
  rect const r        = layout.bounds;
  rr::Painter painter = renderer.painter();
  painter.draw_solid_rect( r, pixel::white() );

  for( auto const& item_layout : layout.items ) {
    if( anim_state.highlighted == item_layout.text ) {
      painter.draw_solid_rect( item_layout.bounds_absolute,
                               gfx::pixel::black() );
      rr::Typer typer =
          renderer.typer( item_layout.bounds_absolute.origin,
                          gfx::pixel::white() );
      typer.write( item_layout.text );
    } else {
      rr::Typer typer =
          renderer.typer( item_layout.bounds_absolute.origin,
                          gfx::pixel::black() );
      typer.write( item_layout.text );
    }
  }

  for( auto const& bar : layout.bars ) {
    auto const r = bar;
    point const start{ .x = r.left(),
                       .y = ( r.bottom() + r.top() ) / 2 };
    painter.draw_horizontal_line( start, r.size.w,
                                  gfx::pixel::black() );
  }
}

} // namespace rn
