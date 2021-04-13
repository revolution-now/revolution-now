/****************************************************************
**unique-coro.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-03-20.
*
* Description: RAII wrapper for coroutine_handle's.
*
*****************************************************************/
#include "unique-coro.hpp"

// C++ standard library
#include <utility>

using namespace std;

namespace rn {

unique_coro::unique_coro( coro::coroutine_handle<> h )
  : h_( h ) {}

unique_coro::~unique_coro() noexcept { destroy(); }

unique_coro::unique_coro( unique_coro&& rhs ) noexcept
  : h_( exchange( rhs.h_, nullptr ) ) {}

unique_coro& unique_coro::operator=(
    unique_coro&& rhs ) noexcept {
  destroy();
  h_ = exchange( rhs.h_, nullptr );
  return *this;
}

void unique_coro::destroy() noexcept {
  if( !h_ ) return;
  exchange( h_, nullptr ).destroy();
}

} // namespace rn
