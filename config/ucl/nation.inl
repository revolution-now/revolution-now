/****************************************************************
* Nation Config File
*****************************************************************/
#ifndef NATION_INL
#define NATION_INL

#include "palette.inl"

namespace rn {

CFG( nation,
  OBJ( dutch,
    FLD( Str,   country_name )
    FLD( Str,   adjective )
    FLD( Str,   article )
    FLD( Color, flag_color )
  )
  OBJ( french,
    FLD( Str,   country_name )
    FLD( Str,   adjective )
    FLD( Str,   article )
    FLD( Color, flag_color )
  )
  OBJ( english,
    FLD( Str,   country_name )
    FLD( Str,   adjective )
    FLD( Str,   article )
    FLD( Color, flag_color )
  )
  OBJ( spanish,
    FLD( Str,   country_name )
    FLD( Str,   adjective )
    FLD( Str,   article )
    FLD( Color, flag_color )
  )
)

}

#endif
