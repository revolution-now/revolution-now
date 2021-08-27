/****************************************************************
* land-view config file
*****************************************************************/
#ifndef LAND_VIEW_INL
#define LAND_VIEW_INL

#include "../../src/font.hpp"
#include "../../src/coord.hpp"

namespace rn {

CFG( land_view,
  OBJ( colonies,
    FLD( e_font, colony_name_font )
    FLD( Delta, colony_name_offset )
  )
)

}

#endif
