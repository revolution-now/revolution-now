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

// clang-format off

/***********************************************************
* Mapping From C++ Types to UCL Types
*
*    C++ type          UCL Enum for Type     Ucl::???_value()
*    ------------------------------------------------------*/
T_(  int,              UCL_INT,              int           );
T_(  bool,             UCL_BOOLEAN,          bool          );
T_(  double,           UCL_FLOAT,            number        );
T_(  std::string,      UCL_STRING,           string        );

/***********************************************************
* Main config file
*
*    Field Name                    Type
*    ------------------------------------------------------*/
C_(  rn                          , object                  ,
F_(    one                       , int                     )
F_(    two                       , std::string             )
F_(    hello                     , int                     )
O_(    fruit                     , object                  ,
F_(      apples                  , int                     )
F_(      oranges                 , int                     )
F_(      description             , std::string             )
O_(      hello                   , object                  ,
F_(        world                 , int                     )
)))

/***********************************************************
* GUI Config File
*
*    Field Name                    Type
*    ------------------------------------------------------*/
C_(  window                      , object                  ,
F_(    game_title                , std::string             )
F_(    game_version              , double                  )
O_(    window_error              , object                  ,
F_(      title                   , std::string             )
F_(      show                    , bool                    )
F_(      x_size                  , int                     )
))

/***********************************************************/
// clang-format on

} // namespace rn
