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

// Revolution Now
#include "fmt-helper.hpp"
#include "util.hpp"

// base-util
#include "base-util/macros.hpp"
#include "base-util/pp.hpp"

// C++ standard library
#include <string_view>
#include <type_traits>
#include <variant>

/****************************************************************
** Algebraic Data Types
*****************************************************************
*  This module defines a macro adt_(...) which is used to declare
*  an Algebraic Data Type.  This is essentially a variant where
*  each alternative is a struct, possibly with members.  Example
*  usage would be:
*
*    adt_( rn::input, state,
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
*          ( double, f ) )       //
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
#define _DEFINE_FORMAT_T( ... ) DEFINE_FORMAT_T( __VA_ARGS__ )
#define _DEFINE_FORMAT_T_( ... ) DEFINE_FORMAT_T_( __VA_ARGS__ )

#define EAT_template( ... ) ( __VA_ARGS__ )

// We need the DEFER here because this will be called recursively
// from another PP_MAP_SEMI below.
#define ADT_MAKE_STRUCT( ns, name, ... )                       \
  namespace ns {                                               \
  struct name {                                                \
    __VA_OPT__( DEFER( PP_MAP_SEMI )( PAIR_TO_DECL_TUPLE,      \
                                      __VA_ARGS__ ) )          \
  };                                                           \
  SWITCH_EMPTY(                                                \
      inline bool operator==( name const& l, name const& r ) { \
        return PP_MAP_AMP( PAIR_TO_CMP_TUPLE, __VA_ARGS__ );   \
      },                                                       \
      inline bool operator==( name const&, name const& ) {     \
        return true;                                           \
      },                                                       \
      __VA_ARGS__ )                                            \
  inline bool operator!=( name const& l, name const& r ) {     \
    return !( l == r );                                        \
  }                                                            \
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

#define ADT_MAKE_STRUCT_T( ns, t_args, name, ... )             \
  namespace ns {                                               \
  template<PP_MAP_COMMAS( PP_ADD_TYPENAME, EXPAND t_args )>    \
  struct name {                                                \
    __VA_OPT__( DEFER( PP_MAP_SEMI )( PAIR_TO_DECL_TUPLE,      \
                                      __VA_ARGS__ ) )          \
  };                                                           \
  template<PP_MAP_COMMAS( PP_ADD_TYPENAME, EXPAND t_args )>    \
  SWITCH_EMPTY(                                                \
      inline bool operator==( name<EXPAND t_args> const& l,    \
                              name<EXPAND t_args> const& r ) { \
        return PP_MAP_AMP( PAIR_TO_CMP_TUPLE, __VA_ARGS__ );   \
      },                                                       \
      inline bool operator==( name<EXPAND t_args> const&,      \
                              name<EXPAND t_args> const& ) {   \
        return true;                                           \
      },                                                       \
      __VA_ARGS__ )                                            \
  template<PP_MAP_COMMAS( PP_ADD_TYPENAME, EXPAND t_args )>    \
  inline bool operator!=( name<EXPAND t_args> const& l,        \
                          name<EXPAND t_args> const& r ) {     \
    return !( l == r );                                        \
  }                                                            \
  }                                                            \
  SWITCH_EMPTY(                                                \
      _DEFINE_FORMAT_T(                                        \
          t_args, (ns::name<EXPAND t_args>),                   \
          "{}::{}{{" JOIN_WITH(                                \
              ", ", PP_MAP_COMMAS( PAIR_TO_FMT_TUPLE,          \
                                   __VA_ARGS__ ) ) "}}",       \
          ::rn::remove_rn_ns( #ns ),                           \
          std::string( #name ) + "<" +                         \
              ::rn::type_list_to_names<EXPAND t_args>() + ">", \
          PP_MAP_COMMAS( PAIR_TO_FMT_O_TUPLE, __VA_ARGS__ ) ), \
      _DEFINE_FORMAT_T_(                                       \
          t_args, (ns::name<EXPAND t_args>), "{}::{}",         \
          ::rn::remove_rn_ns( #ns ),                           \
          std::string( #name ) + "<" +                         \
              ::rn::type_list_to_names<EXPAND t_args>() +      \
              ">" ),                                           \
      __VA_ARGS__ )

#define ADT_MAKE_STRUCT_TUPLE( a ) ADT_MAKE_STRUCT a
#define ADT_MAKE_STRUCT_TUPLE_T( a ) ADT_MAKE_STRUCT_T a

// Arguably most move constructors/assignment members should be
// noexcept, but these variants need to have be that way because
// it allows std::variant to generate more efficient code (in
// some cases it doesn't have to worry about the empty state).
#define ADT_IMPL( ns, name, ... )                          \
  DEFER( PP_MAP )                                          \
  ( ADT_MAKE_STRUCT_TUPLE,                                 \
    PP_MAP_PREPEND_TUPLE( ns::name,                        \
                          __VA_ARGS__ ) ) namespace ns {   \
    using name##_t = std::variant<PP_MAP_PREPEND_NS(       \
        name, PP_MAP_COMMAS( HEAD_TUPLE, __VA_ARGS__ ) )>; \
    static_assert(                                         \
        std::is_nothrow_move_constructible_v<name##_t> );  \
    static_assert(                                         \
        std::is_nothrow_move_assignable_v<name##_t> );     \
  }

#define ADT_T_IMPL( ns, t_args, name, ... ) \
  ADT_T_IMPL_EAT( ns, EAT_##t_args, name, __VA_ARGS__ )

#define ADT_T_IMPL_EAT( ns, t_args, name, ... )               \
  DEFER( PP_MAP )                                             \
  ( ADT_MAKE_STRUCT_TUPLE_T,                                  \
    PP_MAP_PREPEND2_TUPLE( ns::name, t_args,                  \
                           __VA_ARGS__ ) ) namespace ns {     \
    template<PP_MAP_COMMAS( PP_ADD_TYPENAME, EXPAND t_args )> \
    using name##_t = std::variant<JOIN_WITH_TUPLE_EXPAND(     \
        (<EXPAND t_args>),                                    \
        PP_MAP_PREPEND_NS(                                    \
            name,                                             \
            PP_MAP_COMMAS( HEAD_TUPLE, __VA_ARGS__ ) ) )>;    \
  }

/****************************************************************
** Public Interface
*****************************************************************/
#define adt_( ... ) EVAL( ADT_IMPL( __VA_ARGS__ ) )
#define adt_template( ... ) EVAL( ADT_T_IMPL( __VA_ARGS__ ) )

// Use this in namespace ::rn.
#define adt_rn( name, ... )                 \
  } /* close namespace rn. */               \
  EVAL( ADT_IMPL( rn, name, __VA_ARGS__ ) ) \
  /* Re-open namespace rn. */               \
  namespace rn {

// Use this in namespace ::rn::(anonymous).
#define adt_rn_( name, ... )                \
  } /* close namespace (anonymous). */      \
  } /* close namespace rn. */               \
  EVAL( ADT_IMPL( rn, name, __VA_ARGS__ ) ) \
  /* Re-open namespace rn. */               \
  namespace rn {                            \
  /* Re-open namespace (anonymous). */      \
  namespace {

// Use this in namespace ::(anonymous).
#define adt__( name, ... )                  \
  } /* close namespace (anonymous). */      \
  EVAL( ADT_IMPL( rn, name, __VA_ARGS__ ) ) \
  /* Re-open namespace (anonymous). */      \
  namespace {

// Use this in namespace ::(anonymous).
#define adt_template_( ... )            \
  } /* close namespace (anonymous). */  \
  EVAL( ADT_T_IMPL( rn, __VA_ARGS__ ) ) \
  /* Re-open namespace (anonymous). */  \
  namespace {

// Use this in namespace ::(anonymous).
#define adt_template_rn( ... )          \
  } /* close namespace rn. */           \
  EVAL( ADT_T_IMPL( rn, __VA_ARGS__ ) ) \
  /* Re-open namespace rn. */           \
  namespace rn {

// Use this in namespace ::(anonymous).
#define adt_template_rn_( ... )         \
  } /* close namespace (anonymous). */  \
  } /* close namespace rn. */           \
  EVAL( ADT_T_IMPL( rn, __VA_ARGS__ ) ) \
  /* Re-open namespace rn. */           \
  namespace rn {                        \
  /* Re-open namespace (anonymous). */  \
  namespace {

namespace rn {

std::string_view remove_rn_ns( std::string_view sv );

/****************************************************************
** Testing
*****************************************************************/
void test_adt();

} // namespace rn
