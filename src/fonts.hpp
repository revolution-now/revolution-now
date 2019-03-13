/****************************************************************
**fonts.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-22.
*
* Description: Code for handling all things text.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "color.hpp"
#include "sdl-util.hpp"

// C++ standard library
#include <string>

namespace rn {

enum class e_font { _7_12_serif_16pt };

namespace fonts {
constexpr e_font standard = e_font::_7_12_serif_16pt;
}

Texture render_text_line_solid( e_font font, Color fg,
                                std::string_view line );

Texture render_text_line_shadow( e_font font, Color fg,
                                 std::string_view line );

void font_test();

Delta font_rendered_width( e_font font, std::string_view text );

} // namespace rn
