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
#include "enum.hpp"
#include "sdl-util.hpp"

// C++ standard library
#include <string>

namespace rn {

enum class e_(
    font,
    /************************************************************/
    _7_12_serif_16pt, //
    _6x6              //
);

namespace fonts {

e_font standard();

}

Texture render_text_line_solid( e_font font, Color fg,
                                std::string_view line );

Texture render_text_line_shadow( e_font font, Color fg,
                                 std::string_view line );

void font_test();

Delta font_rendered_width( e_font font, std::string_view text );

} // namespace rn
