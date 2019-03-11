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
#include "color.hpp"
#include "geo-types.hpp"
#include "sdl-util.hpp"

namespace rn {

struct TextMarkupInfo {
  Color normal;
  Color highlight;
};

// Will not in any way reformat or re-flow or wrap the text; will
// just render it with spacing/newlines as-is.
void render_text_markup( std::string_view text,
                         Texture const& dest, Coord coord,
                         TextMarkupInfo const& info );

// This will totally re-flow the text with regard to all spacing,
// including newlines, tabs, and inter-word spaces. It will also
// wrap the text to fix in `max_cols`.
void render_text_markup_reflow( std::string_view text,
                                Texture const& dest, Coord coord,
                                TextMarkupInfo const& info,
                                int                   max_cols );

// For testing
void text_render_test();

} // namespace rn
