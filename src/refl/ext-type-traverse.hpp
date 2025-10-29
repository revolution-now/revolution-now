/****************************************************************
**ext-type-traverse.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Type traverse specializations for reflected types.
*
*****************************************************************/
#pragma once

// refl
#include "refl/ext.hpp"

// traverse
#include "traverse/type-ext.hpp"

namespace trv {

// std::tuple of refl::StructFields
template<template<typename> typename O, typename... Ts>
struct TypeTraverse<O, std::tuple<::refl::StructField<Ts>...>> {
  // NOTE: this is non-standard because we don't want to actually
  // traverse the tuple<StructField...>, instead we want to skip
  // straight to the underlying field types.
  using type = trv::list<typename O<
      typename ::refl::StructField<Ts>::type>::type...>;
};

// ReflectedStruct
template<template<typename> typename O,
         ::refl::ReflectedStruct S>
struct TypeTraverse<O, S> {
  using type = TypeTraverse<
      O, std::remove_const_t<
             decltype( ::refl::traits<S>::fields )>>::type;
};

// Variant of ReflectedStructs
template<template<typename> typename O,
         ::refl::ReflectedStruct... Ts>
struct TypeTraverse<O, ::base::variant<Ts...>> {
  using type = trv::list<typename O<Ts>::type...>;
};

// Rds variant
template<template<typename> typename O, typename S>
requires requires { typename S::i_am_rds_variant; }
struct TypeTraverse<O, S> {
  using type = trv::list<typename O<typename S::Base>::type>;
};

// WrapsReflected
template<template<typename> typename O, ::refl::WrapsReflected T>
struct TypeTraverse<O, T> {
  using type = trv::list<typename O<std::remove_cvref_t<
      decltype( std::declval<T>().refl() )>>::type>;
};

} // namespace trv
