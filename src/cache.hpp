/****************************************************************
**cache.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-03-01.
*
* Description: Utilities for caching and memoization.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "core-config.hpp"

// base-util
#include "base-util/non-copyable.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

// C++ standard library
#include <optional>
#include <utility>

namespace rn {

// The memoizer_t classes are classes that wrap two things:
//
//   1) A function pointer (taking either 0 or 1 parameters)
//   2) An "invalidator"
//
// The memoizer_t class is callable and will, when called,
// compute a value by calling the stored function pointer and
// store the result. If the result is already stored the it will
// avoid recomputing it.
//
// However, the "invalidator" object is queried on each call to
// determine whether the cached value should be recomputed. The
// invalidator is a callable and will be called each time the
// value is requested. If the invalidator returns `false` then
// the cached value will be used (assuming there is a cached
// value), and if it returns `true` then the value will be
// recomputed. Therefore, the invalidator will generally hold its
// own state and also check some external state.
//
// Example:
//
//   int calc( int x ) { return x + 1; }
//
//   // This invalidator never invalidates.
//   auto invalidator = [] { return false; };
//
//   auto memoized_calc = memoizer( calc, invalidator );
//
//   auto res1 = memoized_calc();
//   auto res2 = memoized_calc();
template<typename... Args>
class memoizer_t;

// Version for generator functions that take no parameters.
template<typename Invalidator, typename Return>
class memoizer_t<Invalidator, Return ( * )()>
  : public util::non_copy_non_move {
  using Func      = Return ( * )();
  using CacheType = std::optional<Return>;

  Func        generator_;
  Invalidator invalidator_;
  CacheType   cache_;

  bool invalid() { return invalidator_(); }

public:
  memoizer_t( Func generator, Invalidator invalidator )
    : generator_( generator ),
      invalidator_( std::move( invalidator ) ),
      cache_{} {}

  Return const& operator()() {
    if( invalid() || !cache_.has_value() ) cache_ = generator_();
    return *cache_;
  }
};

// Version for generator functions that take one parameter.
template<typename Invalidator, typename Return, typename Arg>
class memoizer_t<Invalidator, Return ( * )( Arg )>
  : public util::non_copy_non_move {
  using Func      = Return ( * )( Arg );
  using CacheType = absl::flat_hash_map<Arg, Return>;

  Func        generator_;
  Invalidator invalidator_;
  CacheType   cache_;

  bool invalid() { return invalidator_(); }

public:
  memoizer_t( Func generator, Invalidator invalidator )
    : generator_( generator ),
      invalidator_( std::move( invalidator ) ) {}

  Return const& operator()( Arg&& arg ) {
    auto it = cache_.find( arg );
    if( invalid() || it == cache_.end() )
      it = cache_.emplace( arg, generator_( arg ) ).first;
    return it->second;
  }
};

template<typename Invalidator, typename Func>
auto memoizer( Func func, Invalidator invalidator ) {
  return memoizer_t<Invalidator, Func>(
      func, std::move( invalidator ) );
}

// Just for testing.
void test_memoizer();

} // namespace rn
