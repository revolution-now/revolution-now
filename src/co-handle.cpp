/****************************************************************
**co-handle.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-03-20.
*
* Description: RAII wrapper for coroutine_handle's.
*
*****************************************************************/
#include "co-handle.hpp"

// Revolution Now
#include "error.hpp"
#include "logging.hpp"

using namespace std;

namespace rn {

unique_coro::unique_coro( coro::coroutine_handle<> h )
  : h_( h ) {
  // Technically this is not necessary, but this code base is
  // constructed such that this should always be true, so it is
  // nice to check it.
  CHECK( h );
  // Technically this is not necessary since a coroutine in its
  // final suspend point can be held and destroyed, but at this
  // time the program should be wrapping any coroutine_handles
  // that are at the final suspend point, and so it is nice to
  // detect that if it happens.
  CHECK( !h.done() );
}

unique_coro::~unique_coro() { destroy(); }

unique_coro::unique_coro( unique_coro&& rhs ) noexcept
  : h_( exchange( rhs.h_, nullptr ) ) {}

unique_coro& unique_coro::operator=(
    unique_coro&& rhs ) noexcept {
  destroy();
  h_ = exchange( rhs.h_, nullptr );
  return *this;
}

void unique_coro::release_and_resume() noexcept {
  CHECK( h_ );
  CHECK( !h_.done() );
  exchange( h_, nullptr )();
}

void unique_coro::destroy() noexcept {
  if( !h_ ) return;
  // TODO: remove this eventually. Here we don't use `lg` to log
  // because that logs to the lua console which is not safe to
  // here since this can run after de-initialization when trig-
  // gered by the destruction of global variables, although that
  // should probably be fixed eventually.
  auto& l = **terminal_logger;
  l.debug( ">>> start destroying an in-flight coroutine." );
  exchange( h_, nullptr ).destroy();
  l.debug( "<<< end   destroying an in-flight coroutine." );
}

} // namespace rn
