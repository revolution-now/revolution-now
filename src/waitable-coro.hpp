/****************************************************************
**waitable-coro.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-09.
*
* Description: Enable waitable to work with coroutines.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "co-scheduler.hpp"
#include "waitable.hpp"

// base
#include "base/co-compat.hpp"

namespace rn {

// Implement co_await_transform extension point for waitable<T>.
template<typename T>
inline waitable<T> co_await_transform( waitable<T> w ) {
  return w;
}

namespace detail {

// The PromiseT is the type (inside the waitable) that will be
// returned by the coroutine that is awaiting on this awaitable.
// The type T is the type that this awaitable will yield.
template<typename PromiseT, typename T>
struct awaitable {
  waitable<T> w_;
  // Promise pointer so that template type can be inferred.
  awaitable( PromiseT*, waitable<T> w ) : w_( std::move( w ) ) {}
  bool await_ready() noexcept { return w_.ready(); }
  void await_suspend( coro::coroutine_handle<> h ) noexcept {
    w_.shared_state()->add_callback(
        [h]( T const& ) { queue_coroutine_handle( h ); } );
    w_.shared_state()->set_exception_callback(
        [h]( std::exception_ptr ) {
          queue_coroutine_handle( h );
        } );
  }
  T await_resume() {
    if( std::exception_ptr eptr = w_.exception(); eptr )
      std::rethrow_exception( eptr );
    return w_.get();
  }
};

// The point of this is that it's not a template, so it can hold
// all the common stuff that does not depend on the type T para-
// meter of the promise type.
struct promise_type_base_base {
  auto initial_suspend() const { return base::suspend_never{}; }
  auto final_suspend() const noexcept {
    return base::suspend_always{};
  }
  // Ensure that this is not copyable. See
  // https://devblogs.microsoft.com/oldnewthing/20210504-00/?p=105176
  // for the arcane reason as to why.
  promise_type_base_base() = default;
  promise_type_base_base( promise_type_base_base const& ) =
      delete;
  void operator=( promise_type_base_base const& ) = delete;
};

template<typename T>
struct promise_type_base : public promise_type_base_base {
  using Base = promise_type_base_base;

  using Base::Base;

  auto get_return_object() {
    return waitable_promise_.waitable();
  }

  void unhandled_exception() {
    waitable_promise_.set_exception( std::current_exception() );
  }

  rn::waitable_promise<T> waitable_promise_{};
};

template<typename T>
struct promise_type final : public promise_type_base<T> {
  using Base = promise_type_base<T>;

  using Base::waitable_promise_;

  promise_type() {
    auto h = coro::coroutine_handle<promise_type>::from_promise(
        *this );
    waitable_promise_.shared_state()->set_coro(
        unique_coro( h ) );
  }

  ~promise_type() noexcept {
    auto h = coro::coroutine_handle<promise_type>::from_promise(
        *this );
    remove_coroutine_if_queued( h );
  }

  void return_value( T const& val ) {
    waitable_promise_.set_value( val );
  }
  void return_value( T&& val ) {
    waitable_promise_.set_value( std::move( val ) );
  }

  template<typename U>
  static auto await_transform( U&& w ) noexcept {
    // The co_await_transform extension point must be implemented
    // for the type U somewhere in the rn:: namespace.
    return awaitable( static_cast<promise_type*>( nullptr ),
                      co_await_transform( std::move( w ) ) );
  }
};

// Specialization for waitable<> that allows us to just use bare
// `co_return` statements instead of `co_return {}`.
template<>
struct promise_type<std::monostate> final
  : public promise_type_base<std::monostate> {
  using Base = promise_type_base<std::monostate>;

  using Base::waitable_promise_;

  promise_type() {
    auto h = coro::coroutine_handle<promise_type>::from_promise(
        *this );
    waitable_promise_.shared_state()->set_coro(
        unique_coro( h ) );
  }

  ~promise_type() noexcept {
    auto h = coro::coroutine_handle<promise_type>::from_promise(
        *this );
    remove_coroutine_if_queued( h );
  }

  void return_void() {
    waitable_promise_.set_value( std::monostate{} );
  }

  template<typename U>
  static auto await_transform( U&& w ) noexcept {
    // The co_await_transform extension point must be implemented
    // for the type U somewhere in the rn:: namespace.
    return awaitable( static_cast<promise_type*>( nullptr ),
                      co_await_transform( std::move( w ) ) );
  }
};

} // namespace detail

} // namespace rn

namespace CORO_NS {

template<typename T, typename... Args>
struct coroutine_traits<::rn::waitable<T>, Args...> {
  using promise_type = ::rn::detail::promise_type<T>;
};

} // namespace CORO_NS
