/****************************************************************
* GUI Config File
*****************************************************************/
#ifndef UI_INL
#define UI_INL

#include "../../src/font.hpp"

namespace rn {

CFG( ui,
  OBJ( window,
    FLD( int, border_width )
    FLD( int, window_padding )
    FLD( Color, border_color )
    FLD( int, ui_padding )
  )

  OBJ( dialog_text,
    FLD( Color, normal )
    FLD( Color, highlighted )
    FLD( int, columns )
  )

  OBJ( menus,
    FLD( rn::W,  first_menu_start )
    FLD( rn::W,  padding )
    FLD( rn::W,  spacing )
    FLD( rn::W,  body_min_width )
    FLD( e_font, font )
    FLD( rn::H,  menu_bar_height )
  )
)

}

#endif
