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
#include "font.hpp"
#include "macros.hpp"
#include "tx.hpp"

// C++ standard library
#include <tuple>

namespace rn {

// Note: these functions cache/reuse results, and so the textures
// returned may be weak references to textures held in the cache.
// However, periodically those textures will be cleaned up, and
// so it is important not to assume anything about the lifetimes
// of the textures referred to by the return values of these
// functions. If you need the texture to last then you should
// clone it.

// Will not in any way reformat or re-flow or wrap the text; will
// just render it with spacing/newlines as-is and with the given
// color. Will ignore markup (will render it literally).
Texture const& render_text( e_font font, Color color,
                            std::string_view text );

// Same as above but uses the default font.
Texture const& render_text( std::string_view text, Color color );

// The struct gives the engine information on how to interpret
// the markup language.
struct TextMarkupInfo {
  Color normal;
  Color highlight;

  // Adds some member functions to make this struct a cache key.
  MAKE_CACHE_KEY( TextMarkupInfo, normal, highlight );
};
NOTHROW_MOVE( TextMarkupInfo );

// Will not in any way reformat or re-flow or wrap the text; will
// just render it with spacing/newlines as-is.
Texture const& render_text_markup( e_font                font,
                                   TextMarkupInfo const& info,
                                   std::string_view      text );

struct TextReflowInfo {
  int max_cols;

  // Adds some member functions to make this struct a cache key.
  MAKE_CACHE_KEY( TextReflowInfo, max_cols );
};
NOTHROW_MOVE( TextReflowInfo );

// This will totally re-flow the text with regard to all spacing,
// including newlines, tabs, and inter-word spaces. It will also
// wrap the text to fix in `max_cols`.
Texture const& render_text_markup_reflow(
    e_font font, TextMarkupInfo const& m_info,
    TextReflowInfo const& r_info, std::string_view text );

/****************************************************************
** Debugging
*****************************************************************/
int text_cache_size();

/****************************************************************
** Testing
*****************************************************************/
// For testing
void text_render_test();

} // namespace rn
