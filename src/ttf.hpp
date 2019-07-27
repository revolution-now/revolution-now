/****************************************************************
**ttf.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-07-27.
*
* Description: Render TTF fonts.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "color.hpp"
#include "coord.hpp"
#include "font.hpp"
#include "sdl-util.hpp" // FIXME: remove

// C++ standard library
#include <string_view>

namespace rn {

Texture ttf_render_text_line_uncached( e_font font, Color fg,
                                       std::string_view line );

/****************************************************************
** Testing
*****************************************************************/
void font_test();

} // namespace rn
