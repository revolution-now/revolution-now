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
#include "coord.hpp"
#include "fonts.hpp"
#include "sdl-util.hpp"

namespace rn {

// FIXME: needs caching beyond what is already done in `fonts`.
// Will not in any way reformat or re-flow or wrap the text; will
// just render it with spacing/newlines as-is and with the given
// color. Will ignore markup (will render it literally).
Texture render_text( e_font font, Color color,
                     std::string_view text );

// FIXME: needs caching beyond what is already done in `fonts`.
// Same as above but uses the default font.
Texture render_text( std::string_view text, Color color );

// The struct gives the engine information on how to interpret
// the markup language.
struct TextMarkupInfo {
  Color normal;
  Color highlight;
};

// FIXME: needs caching beyond what is already done in `fonts`.
// Will not in any way reformat or re-flow or wrap the text; will
// just render it with spacing/newlines as-is.
Texture render_text_markup( e_font                font,
                            TextMarkupInfo const& info,
                            std::string_view      text );

// FIXME: needs caching beyond what is already done in `fonts`.
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
