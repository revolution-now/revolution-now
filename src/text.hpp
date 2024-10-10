/****************************************************************
**text.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-03-10.
*
* Description: Handles rendering larger bodies of text with
*              markup.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "font.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/coord.hpp"
#include "gfx/pixel.hpp"

// C++ standard library
#include <tuple>

namespace rn {

// Will not in any way reformat or re-flow or wrap the text; will
// just render it with spacing/newlines as-is and with the given
// color. Will ignore markup (will render it literally).
void render_text( rr::Renderer& renderer, gfx::point where,
                  e_font font, gfx::pixel color,
                  std::string_view text );

// Same as above but uses the default font.
void render_text( rr::Renderer& renderer, gfx::point where,
                  std::string_view text, gfx::pixel color );

// The struct gives the engine information on how to interpret
// the markup language.
struct TextMarkupInfo {
  gfx::pixel normal    = default_normal_color();
  gfx::pixel highlight = default_highlight_color();
  // Shadowing means that for each pixel in the text, we will
  // render a "shadow" pixel to the right of it and also below
  // it, creating a partial outline or shadow.
  base::maybe<gfx::pixel> shadow = base::nothing;

  static gfx::pixel default_normal_color();
  static gfx::pixel default_highlight_color();
};

// Will not in any way reformat or re-flow or wrap the text; will
// just render it with spacing/newlines as-is.
void render_text_markup( rr::Renderer& renderer,
                         gfx::point where, e_font font,
                         TextMarkupInfo const& info,
                         std::string_view      text );

struct TextReflowInfo {
  int max_cols = std::numeric_limits<int>::max();
};

// This will totally re-flow the text with regard to all spacing,
// including newlines, tabs, and inter-word spaces. It will also
// wrap the text to fix in `max_cols`.
void render_text_markup_reflow( rr::Renderer& renderer,
                                gfx::point where, e_font font,
                                TextMarkupInfo const& m_info,
                                TextReflowInfo const& r_info,
                                std::string_view      text );

// This is not cheap, so ideally it should be called once and the
// result stored, as opposed to calling it every frame.
Delta rendered_text_size( TextReflowInfo const& reflow_info,
                          std::string_view      text );

// Same as above but no reflow.  Will still account for markup.
Delta rendered_text_size_no_reflow( std::string_view text );

// This is useful for debugging/development. It just puts a rec-
// tangle on screen the given lines of text with no frills.
void render_text_overlay_with_anchor(
    rr::Renderer&                   renderer,
    std::vector<std::string> const& lines,
    gfx::point const anchor, e_cdirection const cdirection,
    gfx::pixel const fg_color, gfx::pixel const bg_color );

} // namespace rn
