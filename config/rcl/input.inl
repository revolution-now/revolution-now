/****************************************************************
* User Input System
*****************************************************************/
#ifndef INPUT_INL
#define INPUT_INL

namespace rn {

CFG( input,
  OBJ( controls,
    FLD( int, drag_buffer )
  )

  OBJ( keyboard,
    FLD( bool, use_capslock_as_ctrl )
  )
)

}

#endif
