/****************************************************************
* GUI Config File
*****************************************************************/
#ifndef UI_INL
#define UI_INL

#include "../../src/fonts.hpp"

namespace rn {

CFG( ui,
  OBJ( window,
    FLD( int, border_width )
    FLD( int, window_padding )
    FLD( Color, border_color )
    FLD( int, ui_padding )
  )

  FLD( Str,    game_title )
  FLD( double, game_version )
  FLD( Coord,  coordinates )
  OBJ( window_error,
    FLD( Str,        title )
    FLD( bool,       show )
    FLD( rn::X,      x_size )
    FLD( Vec<Coord>, positions )
  )
  FLD( Vec<rn::W>, widths )

  OBJ( menus,
    FLD( rn::W,  first_menu_start )
    FLD( rn::W,  padding )
    FLD( rn::W,  spacing )
    FLD( rn::W,  body_min_width )
    FLD( e_font, font )
  )
)

}

#endif
