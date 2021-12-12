/****************************************************************
* Fonts
*****************************************************************/
#ifndef FONT_INL
#define FONT_INL

// Revolution Now
#include "../../src/coord.hpp"
#include "../../src/font.hpp"

// base
#include "../../src/base/fs.hpp"

// C++ standard library
#include <unordered_map>

namespace rn {

using FontPathMap       = std::unordered_map<e_font, fs::path>;
using FontSizeMap       = std::unordered_map<e_font, int>;
using FontVertOffsetMap = std::unordered_map<e_font, Y>;

CFG( font,
  FLD( e_font, game_default )
  FLD( e_font, nat_icon     )
  FLD( e_font, small_font   )
  FLD( e_font, main_menu    )
  FLD( FontPathMap,       paths   )
  FLD( FontSizeMap,       sizes   )
  FLD( FontVertOffsetMap, offsets )
)

}

#endif
