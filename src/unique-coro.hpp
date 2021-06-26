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

#include "core-config.hpp"

// base
#include "base/co-compat.hpp"
#include "base/zero.hpp"

namespace rn {

struct unique_coro;
using unique_coro_base =
    base::zero<unique_coro, coro::coroutine_handle<>>;

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
struct unique_coro : unique_coro_base {
  explicit unique_coro( coro::coroutine_handle<> h )
    : unique_coro_base( h ) {}
  MOVABLE_ONLY( unique_coro );

private:
  friend unique_coro_base;
  // Implement base::zero.
  void free_resource() { resource().destroy(); }
};

} // namespace rn
