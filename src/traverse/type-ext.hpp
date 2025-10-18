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
// A template-template type used for doing a compile-time tra-
// versal of the subtypes of a type, with the goal of instanti-
// ating a given template-template on each subtype. Note that,
// like the `traverse` value traverse interface, this is not re-
// cursive; it only goes one level deep in subtypes. That said,
// if you want recursion, you can implement it inside the
// template-template type that you are using for the traversal.
//
// Examples:
//
//   // std::map: two subtypes.
//   template<template<typename> typename O, typename K,
//                                           typename V>
//   struct TypeTraverse<O, std::map<K, V>> {
//     using type = trv::list<
//       typename O<K>::type,
//       typename O<V>::type
//     >;
//   };
//
//   // std::string: no subtypes to traverse.
//   template<template<typename> typename O>
//   struct TypeTraverse<O, std::string> {
//     using type = trv::list<>;
//   };
//
// The below macros produce the above when used like so:
//
//   TRV_TYPE_TRAVERSE( std::map, K, V );
//   TRV_TYPE_TRAVERSE( std::string );
//
// NOTE: We could implement a generic specialization for tem-
// plates that just walks through and visits the template parame-
// ters but we don't do this because then e.g. for std containers
// we'd pick up the allocator type, comparator type, etc., which
// we probably don't want to visit.
template<template<typename> typename O, typename T>
struct TypeTraverse;

template<typename... T>
struct list {};

/****************************************************************
** TRV_TYPE_TRAVERSE
*****************************************************************/
// This is for convenience when the type has a fixed set of tem-
// plate parameters that need to be traversed.

#define TRV_TYPENAME( a )         typename a
#define TRV_TRAVERSE_SUBTYPE( T ) typename O<T>::type

#define TRV_TYPE_TRAVERSE( C, ... )                             \
  EVAL( template<template<typename> typename O __VA_OPT__(      \
            , ) PP_MAP_COMMAS( TRV_TYPENAME, __VA_ARGS__ )>     \
        struct TypeTraverse<O, C __VA_OPT__( <__VA_ARGS__> )> { \
          using type = ::trv::list<PP_MAP_COMMAS(               \
              TRV_TRAVERSE_SUBTYPE, __VA_ARGS__ )>;             \
        } )

/****************************************************************
** Scalars.
*****************************************************************/
template<template<typename> typename O, typename T>
requires std::is_scalar_v<T>
struct TypeTraverse<O, T> {
  using type = trv::list</*no subtypes*/>;
};

/****************************************************************
** Runner.
*****************************************************************/
#define TRV_RUN_TYPE_TRAVERSE( O, T )             \
  [[maybe_unused]] static auto const STRING_JOIN( \
      _, __LINE__ ) = O<T>::type{};

} // namespace trv
