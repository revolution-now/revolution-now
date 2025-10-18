/****************************************************************
**type-ext.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-06.
*
* Description: Machinery for traversing type hierarchies.
*
*****************************************************************/
#pragma once

// base-util
#include "base-util/pp.hpp"

// C++ standard library
#include <type_traits>

namespace trv {

/****************************************************************
** TypeTraverse
*****************************************************************/
// A template-template type used for recursively traversing (at
// compile time) a hierarchy of types, with the goal of instanti-
// ating a given template-template on each one.
//
// Examples:
//
//   // std::string: Non-template type.
//   template<template<typename> typename O>
//   struct TypeTraverse<O, std::string>
//     : traverse_base<
//         O<std::string>
//       >
//     {};
//
//   // std::map: Template type with two template parameters.
//   template<template<typename> typename O, typename U,
//                                           typename V>
//   struct TypeTraverse<O, std::map<U, V>>
//     : traverse_base<
//         TypeTraverse<O, U>,
//         TypeTraverse<O, V>,
//         O<std::map<U, V>>
//       >
//     {};
//
// The below macros produce the above when used like so:
//
//   TRV_TYPE_TRAVERSE( String );
//   TRV_TYPE_TRAVERSE( std::map, U, V );
//
template<template<typename> typename O, typename T>
struct TypeTraverse;

template<typename... T>
struct list {};

template<typename... T>
struct traverse_base {
  using type = list<typename T::type...>;
};

/****************************************************************
** TRV_TYPE_TRAVERSE
*****************************************************************/
// This is for convenience when the type has a fixed set of tem-
// plate parameters that need to be traversed.
#define TRV_TYPENAME( a ) typename a

#define TRV_TRAVERSE_RECURSE( T ) TypeTraverse<O, T>

#define TRV_TYPE_TRAVERSE( C, ... )                             \
  EVAL( template<template<typename> typename O __VA_OPT__(      \
            , ) PP_MAP_COMMAS( TRV_TYPENAME, __VA_ARGS__ )>     \
        struct TypeTraverse<O,                                  \
                            C __VA_OPT__( <__VA_ARGS__> )> : :: \
            trv::traverse_base<                                 \
                PP_MAP_COMMAS( TRV_TRAVERSE_RECURSE,            \
                               __VA_ARGS__ ) __VA_OPT__(, )     \
                    O<C __VA_OPT__( <__VA_ARGS__> )>>{} )

/****************************************************************
** Scalars.
*****************************************************************/
template<template<typename> typename O, typename T>
requires std::is_scalar_v<T>
struct TypeTraverse<O, T> : O<T> {};

/****************************************************************
** Runner.
*****************************************************************/
#define TRV_RUN_TYPE_TRAVERSE( O, T )             \
  [[maybe_unused]] static auto const STRING_JOIN( \
      _, __LINE__ ) = ::trv::TypeTraverse<O, T>::type{};

} // namespace trv
