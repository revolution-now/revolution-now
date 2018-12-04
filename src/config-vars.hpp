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

#include "ucl-config.hpp"

namespace rn {

using str = std::string; // to save typing

// clang-format off

/****************************************************************
* Mapping From C++ Types to UCL Types
*
*    C++ type          UCL Enum for Type     Ucl::???_value()
*    -----------------------------------------------------------*/
T_(  int,              UCL_INT,              int               );
T_(  bool,             UCL_BOOLEAN,          bool              );
T_(  double,           UCL_FLOAT,            number            );
T_(  std::string,      UCL_STRING,           string            );

/****************************************************************
* Main config file
*
*    -----------------------------------------------------------*/
C_(  module, rn                                                ,
/*(                                                            )*/
F_(    int, one                                                )
F_(    str, two                                                )
F_(    int, hello                                              )
/*(                                                            )*/
O_(    object, fruit                                           ,
F_(      int, apples                                           )
F_(      int, oranges                                          )
F_(      str, description                                      )
/*(                                                            )*/
O_(      object, hello                                         ,
F_(        int, world                                          )
)))

/****************************************************************
* GUI Config File
*
*    -----------------------------------------------------------*/
C_(  module, window                                            ,
/*(                                                            )*/
F_(    str,    game_title                                      )
F_(    double, game_version                                    )
/*(                                                            )*/
O_(    object, window_error                                    ,
F_(      str,  title                                           )
F_(      bool, show                                            )
F_(      int,  x_size                                          )
))

/****************************************************************/
// clang-format on

} // namespace rn
