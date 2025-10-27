/****************************************************************
**type-ext-std.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Type-traversing std types.
*
*****************************************************************/
#pragma once

// traverse
#include "type-ext.hpp"

// C++ standard library
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace trv {

// Scalar types.
TRV_TYPE_TRAVERSE( ::std::string );

// NOTE: We could implement a generic specialization for tem-
// plates that just walks through and visits the template parame-
// ters but we don't do this because then e.g. for std containers
// we'd pick up the allocator type, comparator type, etc., which
// we probably don't want to visit.
TRV_TYPE_TRAVERSE( ::std::deque, T );
TRV_TYPE_TRAVERSE( ::std::map, K, V );
TRV_TYPE_TRAVERSE( ::std::unordered_map, K, V );
TRV_TYPE_TRAVERSE( ::std::vector, T );

// std::array
template<template<typename> typename O, typename T, size_t N>
struct TypeTraverse<O, std::array<T, N>> {
  using type = trv::list<typename O<T>::type>;
};

} // namespace trv
