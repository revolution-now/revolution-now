/****************************************************************
* Nation Config File
*****************************************************************/
#ifndef NATION_INL
#define NATION_INL

#include "palette.inl"

namespace rn {

CFG( nation,
  OBJ( dutch,
    FLD( std::string,   country_name )
    FLD( std::string,   adjective )
    FLD( std::string,   article )
    FLD( gfx::pixel,    flag_color )
  )
  OBJ( french,
    FLD( std::string,   country_name )
    FLD( std::string,   adjective )
    FLD( std::string,   article )
    FLD( gfx::pixel,    flag_color )
  )
  OBJ( english,
    FLD( std::string,   country_name )
    FLD( std::string,   adjective )
    FLD( std::string,   article )
    FLD( gfx::pixel,    flag_color )
  )
  OBJ( spanish,
    FLD( std::string,   country_name )
    FLD( std::string,   adjective )
    FLD( std::string,   article )
    FLD( gfx::pixel,    flag_color )
  )
)

}

#endif
