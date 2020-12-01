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
#include "maybe.hpp"

// Abseil
#include "absl/container/node_hash_map.h"

// C++ standard library
#include <utility>

namespace rn {

namespace impl {

// Use this to turn off memoization. In order for this to be re-
// spected you must create your memoizers using one of the stan-
// dard memoizer helper functions, which you should always be
// doing anyway.
inline constexpr bool memoization_enabled = true;

// The memoizer_t classes are classes that wrap two things:
//
//   1) A function pointer (taking either 0 or 1 parameters)
//   2) An "invalidator"
//
// The memoizer_t class is callable and will, when called, com-
// pute a value by calling the stored function pointer and store
// the result. If the result is already stored the it will avoid
// recomputing it.
//
// However, the "invalidator" object is queried on each call to
// determine whether the cached value should be recomputed. The
// invalidator is a callable and will be called each time the
// value is requested. If the invalidator returns `false` then
// the cached value will be used (assuming there is a cached val-
// ue), and if it returns `true` then ALL cached values will be
// cleared and the values will be recomputed as needed. There-
// fore, the invalidator will generally hold its own state and
// also check some external state.
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
//
// WARNING: these memoizers, when returning cached values, will
//          return references to them, and these references
//          could be invalidated while the reference is still
//          being used (when the cache is emptied and value
//          recomputed), so one should never hold references
//          to the returned values for too long.
template<typename... Args>
class memoizer_t;

// Version for generator functions that take no parameters.
template<typename Invalidator, typename Return>
class memoizer_t<Invalidator, Return ( * )()> {
  using Func      = Return ( * )();
  using CacheType = maybe<Return>;

  Func        generator_;
  Invalidator invalidator_;
  CacheType   cache_;

public:
  memoizer_t( Func generator, Invalidator invalidator )
    : generator_( generator ),
      invalidator_( std::move( invalidator ) ),
      cache_{} {}

  Return const& operator()() {
    if( invalidator_() || !cache_.has_value() )
      cache_ = generator_();
    return *cache_;
  }
};

// Base version of memoizer for function of one parameter. This
// is used to avoid duplication below. Note that this class and
// its children must be copyable in order to satisfy range-v3's
// IndirectPredicate concept.
template<typename Invalidator, typename Return, typename Arg>
class memoizer_1_arg_base_t {
public:
  using Func = Return ( * )( Arg );
  // Use node_hash_map because we want pointer stability because
  // our operator() function returns references. WARNING: this
  // still does not guarantee pointer stability after the cache
  // is invalidated.
  using CacheType = absl::node_hash_map<Arg, Return>;

  Func        generator_;
  Invalidator invalidator_;
  CacheType   cache_;

  memoizer_1_arg_base_t( Func          generator,
                         Invalidator&& invalidator )
    : generator_( generator ),
      invalidator_( std::move( invalidator ) ) {}

  Return const& operator()( Arg const& arg ) {
    if( invalidator_() ) cache_.clear();
    auto it = cache_.find( arg );
    if( it == cache_.end() )
      it = cache_.emplace( arg, generator_( arg ) ).first;
    return it->second;
  }
};

// Version for generator functions that take one parameter.
template<typename Invalidator, typename Return, typename Arg>
class memoizer_t<Invalidator, Return ( * )( Arg )>
  : public memoizer_1_arg_base_t<Invalidator, Return, Arg> {
  using Func = Return ( * )( Arg );
  using parent_t =
      memoizer_1_arg_base_t<Invalidator, Return, Arg>;

public:
  memoizer_t( Func generator, Invalidator&& invalidator )
    : parent_t( generator, std::move( invalidator ) ) {}
};

// Version for generator functions that take one parameter as a
// const ref.
template<typename Invalidator, typename Return, typename Arg>
class memoizer_t<Invalidator, Return ( * )( Arg const& )>
  : public memoizer_1_arg_base_t<Invalidator, Return,
                                 Arg const&> {
  using Func = Return ( * )( Arg const& );
  using parent_t =
      memoizer_1_arg_base_t<Invalidator, Return, Arg const&>;

public:
  memoizer_t( Func generator, Invalidator&& invalidator )
    : parent_t( generator, std::move( invalidator ) ) {}
};

inline bool always_invalidator() { return true; }

} // namespace impl

// Use this for memoizers that should never invalidate.
template<typename Func>
auto memoizer( Func func ) {
  if constexpr( impl::memoization_enabled ) {
    auto invalidator = [] { return false; };
    return impl::memoizer_t<decltype( invalidator ), Func>(
        func, std::move( invalidator ) );
  } else {
    return func;
  }
}

template<typename Invalidator, typename Func>
auto memoizer( Func func, Invalidator&& invalidator ) {
  if constexpr( impl::memoization_enabled ) {
    return impl::memoizer_t<Invalidator, Func>(
        func, std::move( invalidator ) );
  } else {
    (void)invalidator;
    return func;
  }
}

// Just for testing.
void test_memoizer();

} // namespace rn
