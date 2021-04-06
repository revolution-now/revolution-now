/****************************************************************
**co-handle.hpp
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
// A std::coroutine_handle is just a wrapper around a raw
// pointer; it does not do reference counting to keep overhead
// low. Therefore, it does not destroy the coroutine upon de-
// struction, since it doesn't have ownership of it (the corou-
// tine it refers to may no longer even exist, as happens when
// the coroutine finishes naturally and does not suspend at the
// final suspend point, or simply if someone else has already de-
// stroyed it).
//
// Therefore, we need an RAII wrapper around the
// std::coroutine_handle, and that is what this type is. However,
// because of the property of std::coroutine_handle mentioned
// above whereby a coroutine can destroy itself after it finishes
// running, this RAII wrapper behaves a bit differently than most
// similar wrappers. First, understand that this wrapper must re-
// lease ownership of the std::coroutine_handle before it can be
// resumed, just in case resuming it causes it to finish natu-
// rally and it releases itself. To enforce this, the method used
// to resume the coroutine (release_and_resume) will, as the name
// says, first release ownership of it before running it. Con-
// versely, if this wrapper holds a non-null coroutine_handle
// then it must be alive, since it will be destroyed if the
// wrapper goes out of scope, and you cannot call destroy on a
// coroutine that has already been destroyed.
//
// Note: in order for this ownership mechanism to work and not
// cause a coroutine leak, it must be the case that, when re-
// sumed, the coroutine will either finish and free itself, or it
// will suspend again and, in doing so, wrap itself in another
// unique_coro.
//
// Another way that this could have been done would have been to
// have the coroutines always suspend at the final suspend point,
// then we would not have to worry about a coroutine destroying
// itself. There are those who advise doing things that way, but
// that seems like it would add additional overhead to the corou-
// tines in general, since all coroutines would be forced to sus-
// pend at least once, and all coroutines would have to suspend
// an additional time.
class unique_coro {
public:
  explicit unique_coro( coro::coroutine_handle<> h );
  ~unique_coro();

  unique_coro( unique_coro const& ) = delete;
  unique_coro& operator=( unique_coro const& ) = delete;
  unique_coro( unique_coro&& rhs ) noexcept;
  unique_coro& operator=( unique_coro&& rhs ) noexcept;

  // First release ownership of the underlying handle, then re-
  // sume the handle.
  void release_and_resume() noexcept;

  operator bool() const noexcept { return bool( h_ ); }

  coro::coroutine_handle<> get() const { return h_; }

private:
  void destroy() noexcept;

  coro::coroutine_handle<> h_;
};

} // namespace rn
