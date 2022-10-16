/****************************************************************
**unique-coro.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-03-19.
*
* Description: RAII wrapper for coroutine_handle's.
*
*****************************************************************/
#pragma once

// base
#include "base/zero.hpp"

// C++ standard library
#include <coroutine>

namespace base {

template<typename Promise>
struct unique_coro;

template<typename Promise>
using unique_coro_base =
    base::zero<unique_coro<Promise>,
               std::coroutine_handle<Promise>>;

// unique_coro
//
// Move-only single-ownership RAII wrapper around a raw coroutine
// handle.
//
// This wrapper assumes that it can always call destroy on the
// coroutine when it itself gets destroyed, and therefore only
// works with coroutines that always suspend at the final_suspend
// point (at which point h.done() will be true).
//
// Therefore, we need an RAII wrapper around the
// std::coroutine_handle, and that is what this type is. However,
// because of the property of std::coroutine_handle mentioned
// above whereby a coroutine can destroy itself after it finishes
// running, we must have the coroutine always suspend at the
// final suspend point.
template<typename Promise>
struct unique_coro : unique_coro_base<Promise> {
  explicit unique_coro( std::coroutine_handle<Promise> h )
    : unique_coro_base<Promise>( h ) {}
  MOVABLE_ONLY( unique_coro );

  Promise const& promise() const {
    return this->resource().promise();
  }

  Promise& promise() { return this->resource().promise(); }

 private:
  friend unique_coro_base<Promise>;
  // Implement base::zero.
  void free_resource() { this->resource().destroy(); }
};

} // namespace base
