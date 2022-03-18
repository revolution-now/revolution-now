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
#include "coord.hpp"
#include "font.hpp"

// render
#include "render/renderer.hpp"

// gfx
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
  gfx::pixel normal    = gfx::pixel::black();
  gfx::pixel highlight = gfx::pixel::white();
  // Shadowing means that for each pixel in the text, we will
  // render a "shadow" pixel to the right of it and also below
  // it, creating a partial outline or shadow.
  gfx::pixel shadowed_text_color   = gfx::pixel::white();
  gfx::pixel shadowed_shadow_color = gfx::pixel::black();
};

// Will not in any way reformat or re-flow or wrap the text; will
// just render it with spacing/newlines as-is.
void render_text_markup( rr::Renderer& renderer,
                         gfx::point where, e_font font,
                         TextMarkupInfo const& info,
                         std::string_view      text );

struct TextReflowInfo {
  int max_cols;
};

// This will totally re-flow the text with regard to all spacing,
// including newlines, tabs, and inter-word spaces. It will also
// wrap the text to fix in `max_cols`.
void render_text_markup_reflow( rr::Renderer& renderer,
                                gfx::point where, e_font font,
                                TextMarkupInfo const& m_info,
                                TextReflowInfo const& r_info,
                                std::string_view      text );

} // namespace rn
