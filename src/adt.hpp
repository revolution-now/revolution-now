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

#define PAIR_TO_DECL( type, var ) type var
#define PAIR_TO_DECL_TUPLE( a ) PAIR_TO_DECL a

#define PAIR_TO_CMP( type, var ) ( l.var == r.var )
#define PAIR_TO_CMP_TUPLE( a ) PAIR_TO_CMP a

// We need the DEFER here because this will be called recursively
// from another PP_MAP_SEMI below.
#define ADT_MAKE_STRUCT( name, ... )                      \
  struct name {                                           \
    __VA_OPT__( DEFER( PP_MAP_SEMI )( PAIR_TO_DECL_TUPLE, \
                                      __VA_ARGS__ ) )     \
  };                                                      \
  bool operator==( name const& l, name const& r ) {       \
    (void)l;                                              \
    (void)r;                                              \
    return true __VA_OPT__(                               \
        &&PP_MAP_AMP( PAIR_TO_CMP_TUPLE, __VA_ARGS__ ) ); \
  }

#define ADT_MAKE_STRUCT_TUPLE( a ) ADT_MAKE_STRUCT a

#define ADT_IMPL( name, ... )                       \
  namespace name {                                  \
  PP_MAP_SEMI( ADT_MAKE_STRUCT_TUPLE, __VA_ARGS__ ) \
  }                                                 \
  using name##_t = std::variant<PP_MAP_PREPEND_NS(  \
      name, PP_MAP_COMMAS( HEAD_TUPLE, __VA_ARGS__ ) )>;

#define ADT( ... ) EVAL( ADT_IMPL( __VA_ARGS__ ) )

} // namespace rn
