/****************************************************************
**extra.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-03.
*
* Description: Higher level rendering utilities.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/pixel.hpp"

namespace rr {

struct Painter;
struct Renderer;
struct TextLayout;

void draw_empty_rect_no_corners( rr::Painter& painter,
                                 gfx::rect box,
                                 gfx::pixel color );

void draw_empty_rect_faded_corners( rr::Renderer& renderer,
                                    gfx::rect box,
                                    gfx::pixel color );

void render_shadow_highlight_border( rr::Renderer& renderer,
                                      gfx::rect rect,
                                      gfx::pixel left_and_bottom,
                                      gfx::pixel top_and_right );

void write_centered( rr::Renderer& renderer, gfx::pixel color_fg,
                     gfx::point center, std::string_view text );

// With a background color.
void write_centered( rr::Renderer& renderer, gfx::pixel color_fg,
                     gfx::pixel color_bg, gfx::point center,
                     std::string_view text );

struct NoisyFillOptions {
  // What percentage of pixels are colored with the variant col-
  // ors. This percentage will be divided equally between the
  // lighter and darker colors. So if this is 0.4 that means that
  // 20% will be the lighter color and 20% (non-overlapping) will
  // be the darker color, with all remaining being the center
  // color. Must be in [0.0, 1.0].
  double intensity = 0.4;
  // The center color will be shaded (highlighted) this many
  // times to get the variant colors.
  int color_divergence = 1;
};

void draw_rect_noisy_filled(
    rr::Renderer& renderer, gfx::rect area,
    gfx::pixel center_color,
    NoisyFillOptions const& options = {} );

// One line of non-markup text rendered with a solid background
// color behind the text in a rect that bounds the text with the
// given amount of padding.
void render_text_line_with_background(
    rr::Renderer& renderer, rr::TextLayout const& text_layout,
    std::string_view line, gfx::oriented_point op,
    gfx::pixel fg_color, gfx::pixel bg_color, int padding,
    bool draw_corners );

} // namespace rr
