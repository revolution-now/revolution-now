/****************************************************************
* Fonts
*****************************************************************/
#ifndef FONT_INL
#define FONT_INL

#include "src/fonts.hpp"

namespace rn {

using FontPathMap       = FlatMap<e_font, fs::path>;
using FontSizeMap       = FlatMap<e_font, int>;
using FontVertOffsetMap = FlatMap<e_font, Y>;

CFG( font,
  FLD( e_font, game_default )
  FLD( FontPathMap,       paths   )
  FLD( FontSizeMap,       sizes   )
  FLD( FontVertOffsetMap, offsets )
)

}

#endif
