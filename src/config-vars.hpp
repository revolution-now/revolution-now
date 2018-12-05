/****************************************************************
**config-vars.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-03.
*
* Description: Describes the names and types of the fields that
*              are expected to be loaded from the config files.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "config-files.hpp"

namespace rn {

// clang-format off

/****************************************************************
* Main config file
*****************************************************************/
CFG( rn,
  FLD( int, one )
  FLD( std::string, two )
  FLD( int, hello )
  OBJ( fruit,
    FLD( int, apples )
    FLD( int, oranges )
    FLD( std::string, description )
    OBJ( hello,
      FLD( int, world )
    )
  )
)

/****************************************************************
* GUI Config File
*****************************************************************/
CFG( window,
  FLD( std::string, game_title )
  FLD( double, game_version )
  OBJ( window_error,
    FLD( std::string, title )
    FLD( bool, show )
    FLD( int, x_size )
  )
)
/****************************************************************/
// clang-format on

} // namespace rn
