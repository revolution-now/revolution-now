/****************************************************************
**hash.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-15.
*
* Description: Utilities for hashing.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// C++ standard library
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>

namespace base {

// Use this for combining fields when specializing std::hash for
// a user-defined type. `seed` would be the has of the first
// field, then call hash_combine once for each subsequent field,
// updating the seed. Then return the seed.
template<class T>
void hash_combine( size_t& seed, T const& o ) {
  std::hash<T> hasher;
  // stackoverflow/questions/2590677/how-do-i-combine-hash-values-in-c0x
  seed ^=
      hasher( o ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
}

// FNV1a c++11 constexpr compile time hash functions, 32 and 64
// bit str should be a null terminated string literal, value
// should be left out e.g hash_32_fnv1a_const( "example" ).
// post: https://notes.underscorediscovery.com/constexpr-fnv1a/

constexpr uint64_t __val_64_const   = 0xcbf29ce484222325;
constexpr uint64_t __prime_64_const = 0x100000001b3;

inline constexpr uint64_t hash_64_fnv1a_const(
    char const* const str,
    uint64_t const    value = __val_64_const ) noexcept {
  return ( str[0] == '\0' )
             ? value
             : hash_64_fnv1a_const(
                   &str[1], ( value ^ uint64_t( str[0] ) ) *
                                __prime_64_const );
}

// This uses a trick, based on an idea here:
//
//   https://stackoverflow.com/questions/35941045/
//       can-i-obtain-c-type-names-in-a-constexpr-way
//
// To get a unique identifier for a type that is constexpr
// (typeid fields are not yet constexpr). The idea is that by
// specializing a function with the type then that type will be-
// come part of the mangled function name, so therefore if we
// hash that name we get a unique (hopefully) id.
//
// IMPORTANT: Because this is a hash-based approach, it is not
// guaranteed to be unique.
template<class T>
constexpr std::size_t compute_type_hash() {
  return hash_64_fnv1a_const( __PRETTY_FUNCTION__ );
}

template<typename T>
constexpr std::size_t type_hash = compute_type_hash<T>();

} // namespace base
