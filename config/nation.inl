/****************************************************************
* Nation Config File
*****************************************************************/
#ifndef NATION_INL
#define NATION_INL

#include "palette.inl"

namespace rn {

CFG( nation,
  OBJ( dutch,
    FLD( Str, country_name )
    LNK( flag_color, palette.orange.sat2.lum9 )
  )
  OBJ( french,
    FLD( Str, country_name )
    LNK( flag_color, palette.blue.sat2.lum6 )
  )
  OBJ( english,
    FLD( Str, country_name )
    LNK( flag_color, palette.red.sat2.lum7 )
  )
  OBJ( spanish,
    FLD( Str, country_name )
    LNK( flag_color, palette.yellow.sat2.lum13 )
  )
)

}

#endif
