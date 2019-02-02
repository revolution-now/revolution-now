/****************************************************************
**adt.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-18.
*
* Description: Algebraic Data Types.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// base-util
#include "base-util/pp.hpp"

// C++ standard library
#include <string_view>
#include <variant>

/****************************************************************
** Algebraic Data Types
*****************************************************************
*  This module defines a macro ADT(...) which is used to declare
*  an Algebraic Data Type.  This is essentially a variant where
*  each alternative is a struct, possibly with members.  Example
*  usage would be:
*
*    ADT( rn::input, state,
*        ( steady ),             //
*        ( stopped ),            //
*        ( paused,               //
*          ( double, percent ),  //
*          ( std::string, msg ), //
*          ( int, y ) ),         //
*        ( starting,             //
*          ( int, x ),           //
*          ( int, y ) ),         //
*        ( ending,               //
*          ( double, f )         //
*          )                     //
*    )
*
*    input::state_t my_state; // my_state is a std::variant
*    my_state = input::state::starting{5, 6};
*
*    input::state_t my_state2 = my_state;
*    assert( my_state == my_state2 );
*
*    std::get<input::state::starting>( my_state ).x = 3;
*
*    std::visit( ..., my_state );
*
*    fmt::format( "my_state: {}", my_state );
*
*  In addition to declaring the variant and its alternatives, it
*  will also define equality operators for each alternative
*  (which just do a member-wise comparison) and will declare
*  {fmt} formatting overloads for each alternative, allowing one
*  to format an ADT type given an {fmt} overload for general
*  variants and given that all member variables are formattable.
*/

// (int, x) --> int x
#define PAIR_TO_DECL( type, var ) type var
#define PAIR_TO_DECL_TUPLE( a ) PAIR_TO_DECL a

// (int, x) --> (l.x == r.x)
#define PAIR_TO_CMP( type, var ) ( l.var == r.var )
#define PAIR_TO_CMP_TUPLE( a ) PAIR_TO_CMP a

// (int, x) --> "x={}"
#define PAIR_TO_FMT( type, var ) #var "={}"
#define PAIR_TO_FMT_TUPLE( a ) PAIR_TO_FMT a

// (int, x) --> o.x
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
  namespace ns {                                               \
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
          ns::name,                                            \
          "{}::{}{{" JOIN_WITH(                                \
              ", ", PP_MAP_COMMAS( PAIR_TO_FMT_TUPLE,          \
                                   __VA_ARGS__ ) ) "}}",       \
          ::rn::remove_rn_ns( #ns ), #name,                    \
          PP_MAP_COMMAS( PAIR_TO_FMT_O_TUPLE, __VA_ARGS__ ) ); \
      , _DEFINE_FORMAT_( ns::name, "{}::{}",                   \
                         ::rn::remove_rn_ns( #ns ), #name );   \
      , __VA_ARGS__ )

#define ADT_MAKE_STRUCT_TUPLE( a ) ADT_MAKE_STRUCT a

#define ADT_IMPL( ns, name, ... )                          \
  DEFER( PP_MAP )                                          \
  ( ADT_MAKE_STRUCT_TUPLE,                                 \
    PP_MAP_PREPEND_TUPLE( ns::name,                        \
                          __VA_ARGS__ ) ) namespace ns {   \
    using name##_t = std::variant<PP_MAP_PREPEND_NS(       \
        name, PP_MAP_COMMAS( HEAD_TUPLE, __VA_ARGS__ ) )>; \
  }

#define ADT( ns, ... ) EVAL( ADT_IMPL( ns, __VA_ARGS__ ) )

namespace rn {

std::string_view remove_rn_ns( std::string_view sv );

} // namespace rn
