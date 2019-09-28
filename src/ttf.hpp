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
#include "tx.hpp"

// C++ standard library
#include <string_view>

namespace rn {

Texture ttf_render_text_line_uncached( e_font font, Color fg,
                                       std::string_view line );

struct FontTTFInfo {
  SY height;
};
NOTHROW_MOVE( FontTTFInfo );

FontTTFInfo const& ttf_get_font_info( e_font font );

/****************************************************************
** Testing
*****************************************************************/
void font_test();

} // namespace rn
