/****************************************************************
* Fonts
*****************************************************************/
#ifndef FONT_INL
#define FONT_INL

#include "../../src/font.hpp"

namespace rn {

using FontPathMap       = FlatMap<e_font, fs::path>;
using FontSizeMap       = FlatMap<e_font, int>;
using FontVertOffsetMap = FlatMap<e_font, Y>;

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
