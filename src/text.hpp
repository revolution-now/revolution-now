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
#include "fonts.hpp"
#include "geo-types.hpp"
#include "sdl-util.hpp"

namespace rn {

struct TextMarkupInfo {
  Color normal;
  Color highlight;
};

// Will not in any way reformat or re-flow or wrap the text; will
// just render it with spacing/newlines as-is.
Texture render_text_markup( e_font                font,
                            TextMarkupInfo const& info,
                            std::string_view      text );

// This will totally re-flow the text with regard to all spacing,
// including newlines, tabs, and inter-word spaces. It will also
// wrap the text to fix in `max_cols`.
Texture render_text_markup_reflow( e_font                font,
                                   TextMarkupInfo const& info,
                                   std::string_view      text,
                                   int max_cols );

// For testing
void text_render_test();

} // namespace rn
