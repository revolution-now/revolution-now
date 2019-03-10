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
#include "geo-types.hpp"
#include "sdl-util.hpp"

namespace rn {

// Will not in any way reformat or re-flow or wrap the text.
void render_text_markup( std::string_view text,
                         Texture const& dest, Coord coord );

// Will flatten the text onto one line, then wrap it to within
// `max_cols` columns.
void render_text_markup_wrap( std::string_view text,
                              Texture const& dest, Coord coord,
                              int max_cols );

// For testing
void text_render_test();

} // namespace rn
