/****************************************************************
* Main config file
*****************************************************************/
#ifndef RN_INL
#define RN_INL

#include "../../src/font.hpp"
#include "../../src/nation.hpp"
#include "../../src/time.hpp"

namespace rn {

CFG( rn,
  FLD( e_nation, player_nation )
  FLD( int,      target_frame_rate )
  FLD( bool,     wait_for_vsync )
  FLD( double,   depixelate_per_frame )
  FLD( double,   ideal_tile_angular_size )

  OBJ( main_window,
    FLD( std::string, title )
  )

  OBJ( viewport,
    FLD( double, pan_speed )
    FLD( double, zoom_min )
    FLD( double, zoom_speed )
    FLD( double, zoom_accel_coeff )
    FLD( double, zoom_accel_drag_coeff )
    FLD( double, pan_accel_init_coeff )
    FLD( double, pan_accel_drag_init_coeff )

    FLD( bool, can_reveal_space_around_map )
  )

  OBJ( console,
    FLD( e_font, font )
  )

  OBJ( power,
    FLD( Seconds, time_till_slow_fps )
  )
)

}

#endif
