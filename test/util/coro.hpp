/****************************************************************
**coro.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-04-07.
*
* Description: Coro-related helpers for unit testing.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/wait.hpp"

// base
#include "src/base/error.hpp"

namespace rn {

template<typename T>
T co_await_test( wait<T> const w ) {
  BASE_CHECK( !w.exception() );
  BASE_CHECK( w.ready() );
  return std::move( *w );
}

} // namespace rn
