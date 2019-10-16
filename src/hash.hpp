/****************************************************************
**hash.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-09.
*
* Description: All things hashing.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Abseil
#include "absl/hash/hash.h"

// C++ standard library
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace rn {

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

// Pass as the third template argument to hash map so that we can
// use enum classes as the keys in maps.
struct EnumClassHash {
  template<class T>
  std::size_t operator()( T t ) const noexcept {
    return static_cast<std::size_t>( t );
  }
};

} // namespace rn
