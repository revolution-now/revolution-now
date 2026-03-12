/****************************************************************
**co-maybe.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-31.
*
* Description: Setup for using `maybe` with coroutines.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "error.hpp"
#include "maybe.hpp"

// C++ standard library
#include <coroutine>

// FIXME: maybes, expecteds, etc. should probably not be used
// with coroutines until it is verified in godbolt that both
// clang and gcc can optimize away all of the coroutine state
// heap allocations. At the time of writing, clang seems to be
// able to do it but not gcc. See https://godbolt.org/z/MKsPPG

namespace base::detail {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
template<typename T>
struct maybe_promise_type;

template<typename T>
struct maybe_holder;

/****************************************************************
** Awaiters.
*****************************************************************/
struct maybe_await_bool {
  bool const b_ = {};

  constexpr bool await_ready() const noexcept { return b_; }

  template<typename Promise>
  bool await_suspend(
      std::coroutine_handle<Promise> ) const noexcept {
    // One of the awaited maybes was nothing, thus suspend. We
    // will never resume this coroutine, and the outer return ob-
    // ject will destroy the coroutine frame safely from outside.
    return true;
  }

  constexpr void await_resume() const noexcept {}
};

template<typename T>
struct maybe_await {
  maybe<T> const* const o_;

  bool await_ready() const noexcept { return o_->has_value(); }

  template<typename Promise>
  bool await_suspend(
      std::coroutine_handle<Promise> ) const noexcept {
    // One of the awaited maybes was nothing, thus suspend. We
    // will never resume this coroutine, and the outer return ob-
    // ject will destroy the coroutine frame safely from outside.
    return true;
  }

  T await_resume() const noexcept { return o_->value(); }
};

template<typename T>
struct maybe_await<T&> {
  maybe<T&> const* const o_;

  bool await_ready() const noexcept { return o_->has_value(); }

  template<typename Promise>
  bool await_suspend(
      std::coroutine_handle<Promise> ) const noexcept {
    // One of the awaited maybes was nothing, thus suspend. We
    // will never resume this coroutine, and the outer return ob-
    // ject will destroy the coroutine frame safely from outside.
    return true;
  }

  T& await_resume() const noexcept { return o_->value(); }
};

/****************************************************************
** Owning coroutine return object.
*****************************************************************/
// This is what get_return_object() returns. It owns the handle
// until the compiler performs the implicit conversion to may-
// be<T> at the coroutine function's return point.
template<typename T>
struct maybe_holder {
  using promise_t = maybe_promise_type<T>;
  using handle_t  = std::coroutine_handle<promise_t>;

  handle_t h_ = {};

  explicit maybe_holder( handle_t const h ) noexcept : h_( h ) {}

  maybe_holder( maybe_holder&& other ) noexcept
    : h_( std::exchange( other.h_, {} ) ) {}

  maybe_holder( maybe_holder const& )            = delete;
  maybe_holder& operator=( maybe_holder const& ) = delete;
  maybe_holder& operator=( maybe_holder&& )      = delete;

  ~maybe_holder() {
    if( h_ ) h_.destroy();
  }

  operator maybe<T>() && noexcept {
    // Extract result, then destroy safely from outside of the
    // coroutine frame (coroutine will be suspended).
    maybe<T> out = std::move( h_.promise().result_ );
    h_.destroy();
    h_ = {};
    return out;
  }
};

// TODO: Add support for maybe-ref promise types here. Although
// we can co_await a maybe-ref in a maybe coroutine, we can't de-
// clare coroutines with maybe-ref return types (promise types).
// Currently it doesn't work because maybe-refs are not assigna-
// ble, and we expect the pre-created return value to be assigna-
// ble. However, this could probably be implemented by creating a
// separate maybe_holder for maybe_refs that holds a pointer to T
// instead of a maybe.
template<typename T>
struct maybe_promise_type {
  maybe<T> result_;

  constexpr maybe_promise_type() = default;

  auto initial_suspend() noexcept {
    return std::suspend_never{};
  }

  using holder = maybe_holder<T>;
  // Take advantage of the fact that the get_return_object func-
  // tion on the promise is allowed to return a type that is im-
  // plicitly convertible to the result. This type will be re-
  // sponsible for extracting the result from the promise and
  // also destroying the coroutine.
  //
  // Note this also requires that compiler defer the implicit
  // conversion of the return object until the point of return.
  // Some versions of MSVC may not do this (although they are
  // supposed to in the future); if the implicit conversion is
  // done eagerly then what is done here is probably undefined
  // behavior in that the maybe_holder would be immediately con-
  // verted to a maybe and the coroutine frame destroyed.
  holder get_return_object() noexcept {
    using handle_t = std::coroutine_handle<maybe_promise_type>;
    return holder{ handle_t::from_promise( *this ) };
  }

  void return_value( T const& val ) noexcept { result_ = val; }

  void return_value( T&& val ) noexcept {
    result_ = std::move( val );
  }

  auto final_suspend() noexcept {
    // Suspend at the end so the owning maybe_holder can inspect
    // the promise and destroy the frame from outside.
    return std::suspend_always{};
  }

  // Doing nothing in this method could be dubious -- I am told
  // that it would swallow any exception that happens in the
  // coroutine and convert it to a `nothing` result... not sure
  // that is what we want. So we will check-fail.
  void unhandled_exception() noexcept {
    FATAL( "unhandled exception in maybe coroutine." );
  }

  template<typename U>
  static auto await_transform( maybe<U> const& o ) noexcept {
    return maybe_await{ &o };
  }

  template<typename U>
  static auto await_transform( maybe<U&> const& o ) noexcept {
    return maybe_await{ &o };
  }

  static auto await_transform( nothing_t ) noexcept {
    return maybe_await_bool{ false };
  }

  static auto await_transform( bool b ) noexcept {
    return maybe_await_bool{ b };
  }
};

} // namespace base::detail

/****************************************************************
** std extension point.
*****************************************************************/
namespace std {
template<typename T, typename... Args>
struct coroutine_traits<::base::maybe<T>, Args...> {
  using promise_type = ::base::detail::maybe_promise_type<T>;
};
} // namespace std