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

// C++ standard library
#include <functional>

namespace base {

template<class T>
void hash_combine( size_t& seed, T const& o ) {
  std::hash<T> hasher;
  // stackoverflow/questions/2590677/how-do-i-combine-hash-values-in-c0x
  seed ^=
      hasher( o ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
}

} // namespace base
