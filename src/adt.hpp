/****************************************************************
**adt.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-18.
*
* Description: Abstract Data Types.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// base-util
#include "base-util/pp.hpp"

// C++ standard library
#include <variant>

namespace rn {

#define ADT_ADD_SEMI( a ) a;
#define ADT_JOIN_SEMIS( ... ) \
  __VA_OPT_( PP_MAP( ADT_ADD_SEMI, __VA_ARGS__ ) )

#define ADT_MAKE_STRUCT( name, ... ) \
  struct name {                      \
    ADT_JOIN_SEMIS( __VA_ARGS__ )    \
  }

#define ADT( name, v1, v2, v3, v4 )             \
  EVAL( ADT_MAKE_STRUCT v1; ADT_MAKE_STRUCT v2; \
        ADT_MAKE_STRUCT v3; ADT_MAKE_STRUCT v4; \
        using name =                            \
            std::variant<HEAD v1, HEAD v2, HEAD v3, HEAD v4> );

ADT( MyAdtType,
     ( case_a ),               //
     ( case_b ),               //
     ( case_c, int x, int y ), //
     ( case_d, double f )      //
)

} // namespace rn
