/****************************************************************
**extra.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-03.
*
* Description: Higher level rendering utilities.
*
*****************************************************************/
#include "extra.hpp"

// render
#include "painter.hpp"
#include "renderer.hpp"

using namespace std;

namespace rr {

namespace {

using ::gfx::oriented_point;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void draw_empty_rect_no_corners( Painter& painter,
                                 rect const box,
                                 pixel const color ) {
  // Left.
  {
    point const start = box.nw();
    point const end   = box.sw();
    painter.draw_vertical_line(
        start + size{ .h = 1 },
        std::max( 0, end.y - start.y - 2 + 1 ), color );
  }
  // Right.
  {
    point const start = box.ne();
    point const end   = box.se();
    painter.draw_vertical_line(
        start + size{ .h = 1 },
        std::max( 0, end.y - start.y - 2 + 1 ), color );
  }
  // Top.
  {
    point const start = box.nw();
    point const end   = box.ne();
    painter.draw_horizontal_line(
        start + size{ .w = 1 },
        std::max( 0, end.x - start.x - 2 + 1 ), color );
  }
  // Bottom.
  {
    point const start = box.sw();
    point const end   = box.se();
    painter.draw_horizontal_line(
        start + size{ .w = 1 },
        std::max( 0, end.x - start.x - 2 + 1 ), color );
  }
}

void draw_empty_rect_faded_corners( Renderer& renderer,
                                    gfx::rect box,
                                    gfx::pixel color ) {
  {
    Painter painter = renderer.painter();
    draw_empty_rect_no_corners( painter, box, color );
  }
  {
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .5 );
    Painter painter = renderer.painter();
    painter.draw_empty_rect( box, color );
  }
}

void render_shadow_highlight_border(
    Renderer& renderer, rect const rect,
    pixel const left_and_bottom, pixel const top_and_right ) {
  Painter painter = renderer.painter();
  painter.draw_horizontal_line( rect.nw().moved_up(),
                                rect.size.w + 1, top_and_right );
  painter.draw_horizontal_line( rect.sw(), rect.size.w,
                                left_and_bottom );
  painter.draw_vertical_line( rect.nw().moved_left(),
                              rect.size.h + 1, left_and_bottom );
  painter.draw_vertical_line( rect.ne(), rect.size.h,
                              top_and_right );
  pixel const mixed = mix( left_and_bottom, top_and_right );
  painter.draw_point( rect.nw().moved_up().moved_left(), mixed );
  painter.draw_point( rect.se(), mixed );
}

void write_centered( Renderer& renderer, pixel const color_fg,
                     point const center,
                     string_view const text ) {
  Typer typer = renderer.typer();
  typer.set_color( color_fg );
  size const text_size = typer.dimensions_for_line( text );
  rect const text_rect = gfx::centered_on( text_size, center );
  typer.set_position( text_rect.nw() );
  typer.write( text );
}

void write_centered( Renderer& renderer, pixel const color_fg,
                     pixel const color_bg, point const center,
                     string_view const text ) {
  Typer typer = renderer.typer();
  typer.set_color( color_bg );
  size const text_size = typer.dimensions_for_line( text );
  rect const text_rect = gfx::centered_on( text_size, center );
  typer.set_position( text_rect.nw() + size{ .w = 1 } );
  typer.write( text );
  typer.set_position( text_rect.nw() + size{ .h = 1 } );
  typer.write( text );
  typer.set_color( color_fg );
  renderer.typer( text_rect.nw(), color_fg ).write( text );
}

void draw_rect_noisy_filled( Renderer& renderer, rect const area,
                             pixel const center_color,
                             NoisyFillOptions const& options ) {
  renderer.painter().draw_solid_rect(
      rect{ .origin = {},
            .size   = renderer.logical_screen_size() },
      center_color );
  {
    pixel const noise_color =
        center_color.shaded( options.color_divergence );
    SCOPED_RENDERER_MOD_MUL( painter_mods.depixelate.stage,
                             1.0 - options.intensity / 2.0 )
    renderer.painter().draw_solid_rect( area, noise_color );
  }
  {
    pixel const noise_color =
        center_color.highlighted( options.color_divergence );
    SCOPED_RENDERER_MOD_MUL( painter_mods.depixelate.stage,
                             options.intensity / 2.0 )
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.inverted,
                             true );
    renderer.painter().draw_solid_rect( area, noise_color );
  }
}

void render_text_line_with_background(
    Renderer& renderer, TextLayout const& text_layout,
    std::string_view const line, oriented_point const op,
    pixel const color_fg, pixel const color_bg,
    int const padding, bool draw_corners ) {
  size const text_dims =
      renderer.typer( text_layout ).dimensions_for_line( line );
  size const padded_dims = [&] {
    size sz = text_dims;
    sz.w += padding * 2;
    sz.h += padding * 2;
    return sz;
  }();
  point const padded_p = gfx::find_placement( op, padded_dims );
  rect const bg{ .origin = padded_p, .size = padded_dims };
  Painter painter = renderer.painter();
  if( draw_corners ) {
    painter.draw_solid_rect( bg, color_bg );
  } else {
    painter.draw_solid_rect( bg.with_edges_removed(), color_bg );
    draw_empty_rect_no_corners( painter, bg.with_dec_size(),
                                color_bg );
  }
  point const text_p = gfx::centered_in( text_dims, bg );
  Typer typer        = renderer.typer( text_p, color_fg );
  typer.write( line );
}

} // namespace rr
