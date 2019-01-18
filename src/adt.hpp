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

// ADT( MyAdtType,
//     ( case_a ),               //
//     ( case_b ),               //
//     ( case_c, int x, int y ), //
//     ( case_d, double f; )     //
//)

#define ADT_MAKE_STRUCT( name, ... )        \
  struct name {                             \
    __VA_OPT__( JOIN_SEMIS( __VA_ARGS__ ) ) \
  }

#define ADT_MAKE_STRUCT_TUPLE( a ) ADT_MAKE_STRUCT a;
#define HEAD_TUPLE( a ) HEAD a

#define ADT_NON_EVAL( name, ... )                   \
  PP_MAP_SEMI( ADT_MAKE_STRUCT_TUPLE, __VA_ARGS__ ) \
  using name =                                      \
      std::variant<PP_MAP_COMMAS( HEAD_TUPLE, __VA_ARGS__ )>;

#define ADT( ... ) EVAL( ADT_NON_EVAL( __VA_ARGS__ ) )

} // namespace rn
