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

#define PAIR_TO_DECL( type, var ) type var
#define PAIR_TO_DECL_TUPLE( a ) PAIR_TO_DECL a

#define PAIR_TO_CMP( type, var ) ( l.var == r.var )
#define PAIR_TO_CMP_TUPLE( a ) PAIR_TO_CMP a

#define PAIR_TO_FMT( type, var ) #var "={}"
#define PAIR_TO_FMT_TUPLE( a ) PAIR_TO_FMT a

#define PAIR_TO_FMT_O( type, var ) o.var
#define PAIR_TO_FMT_O_TUPLE( a ) PAIR_TO_FMT_O a

// These definitions can be commented to debug the PP output
// without it getting filled with the contents of these macros,
// which are probably not under scrutiny here.
#define _DEFINE_FORMAT( ... ) DEFINE_FORMAT( __VA_ARGS__ )
#define _DEFINE_FORMAT_( ... ) DEFINE_FORMAT_( __VA_ARGS__ )

// We need the DEFER here because this will be called recursively
// from another PP_MAP_SEMI below.
#define ADT_MAKE_STRUCT( ns, name, ... )                       \
  namespace rn::ns {                                           \
  struct name {                                                \
    __VA_OPT__( DEFER( PP_MAP_SEMI )( PAIR_TO_DECL_TUPLE,      \
                                      __VA_ARGS__ ) )          \
  };                                                           \
  SWITCH_EMPTY(                                                \
      bool operator==( name const& l, name const& r ) {        \
        return PP_MAP_AMP( PAIR_TO_CMP_TUPLE, __VA_ARGS__ );   \
      },                                                       \
      bool operator==( name const&, name const& ) {            \
        return true;                                           \
      },                                                       \
      __VA_ARGS__ )                                            \
  }                                                            \
  SWITCH_EMPTY(                                                \
      _DEFINE_FORMAT(                                          \
          rn::ns::name,                                        \
          "{}::{}{{" JOIN_WITH(                                \
              ", ", PP_MAP_COMMAS( PAIR_TO_FMT_TUPLE,          \
                                   __VA_ARGS__ ) ) "}}",       \
          #ns, #name,                                          \
          PP_MAP_COMMAS( PAIR_TO_FMT_O_TUPLE, __VA_ARGS__ ) ); \
      , _DEFINE_FORMAT_( rn::ns::name, "{}::{}", #ns, #name ); \
      , __VA_ARGS__ )

#define ADT_MAKE_STRUCT_TUPLE( a ) ADT_MAKE_STRUCT a

#define ADT_IMPL( name, ... )                                  \
  DEFER( PP_MAP )                                              \
  ( ADT_MAKE_STRUCT_TUPLE,                                     \
    PP_MAP_PREPEND_TUPLE( name, __VA_ARGS__ ) ) namespace rn { \
    using name##_t = std::variant<PP_MAP_PREPEND_NS(           \
        name, PP_MAP_COMMAS( HEAD_TUPLE, __VA_ARGS__ ) )>;     \
  }

#define ADT( ... ) EVAL( ADT_IMPL( __VA_ARGS__ ) )

namespace rn {

void adt_test();

} // namespace rn
