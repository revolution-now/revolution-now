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

namespace rn {

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
class unique_coro {
public:
  explicit unique_coro( coro::coroutine_handle<> h );
  ~unique_coro() noexcept;

  unique_coro( unique_coro const& ) = delete;
  unique_coro& operator=( unique_coro const& ) = delete;
  unique_coro( unique_coro&& rhs ) noexcept;
  unique_coro& operator=( unique_coro&& rhs ) noexcept;

private:
  void destroy() noexcept;

  coro::coroutine_handle<> h_;
};

} // namespace rn
