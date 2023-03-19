/****************************************************************
**co-wait.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-09.
*
* Description: Enable wait to work with coroutines.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "co-scheduler.hpp"
#include "wait.hpp"

// C++ standard library
#include <coroutine>

namespace rn {

// Implement co_await_transform extension point for wait<T>.
template<typename T>
inline wait<T> co_await_transform( wait<T> w ) {
  return w;
}

namespace detail {

// The PromiseT is the type (inside the wait) that will be re-
// turned by the coroutine that is awaiting on this awaitable.
// The type T is the type that this awaitable will yield.
template<typename PromiseT, typename T>
struct awaitable {
  wait<T> w_;
  // Promise pointer so that template type can be inferred.
  awaitable( PromiseT*, wait<T> w ) : w_( std::move( w ) ) {}
  bool await_ready() noexcept {
    return w_.ready() || w_.has_exception();
  }
  void await_suspend( std::coroutine_handle<> h ) noexcept {
    w_.state()->add_callback(
        [h]( T const& ) { queue_cpp_coroutine_handle( h ); } );
    w_.state()->set_exception_callback(
        [h]( std::exception_ptr ) {
          queue_cpp_coroutine_handle( h );
        } );
  }
  T await_resume() {
    if( std::exception_ptr eptr = w_.exception(); eptr )
      std::rethrow_exception( eptr );
    return std::move( w_.get() );
  }
};

// The point of this is that it's not a template, so it can hold
// all the common stuff that does not depend on the type T para-
// meter of the promise type.
struct promise_type_base_base {
  auto initial_suspend() const { return std::suspend_never{}; }
  auto final_suspend() const noexcept {
    return std::suspend_always{};
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

  auto get_return_object() { return wait_promise_.wait(); }

  void unhandled_exception() {
    wait_promise_.set_exception( std::current_exception() );
  }

  rn::wait_promise<T> wait_promise_{};
};

template<typename T>
struct promise_type final : public promise_type_base<T> {
  using Base = promise_type_base<T>;

  using Base::wait_promise_;

  promise_type() {
    auto h = std::coroutine_handle<promise_type>::from_promise(
        *this );
    wait_promise_.state()->set_coro(
        base::unique_coro<promise_type>( h ) );
  }

  ~promise_type() noexcept {
    auto h = std::coroutine_handle<promise_type>::from_promise(
        *this );
    remove_cpp_coroutine_if_queued( h );
  }

  void return_value( T const& val ) {
    wait_promise_.set_value( val );
  }
  void return_value( T&& val ) {
    wait_promise_.set_value( std::move( val ) );
  }

  template<typename U>
  static auto await_transform( U&& w ) noexcept {
    // The co_await_transform extension point must be implemented
    // for the type U somewhere in the rn:: namespace.
    return awaitable( static_cast<promise_type*>( nullptr ),
                      co_await_transform( std::move( w ) ) );
  }
};

// Specialization for wait<> that allows us to just use bare
// `co_return` statements instead of `co_return {}`.
template<>
struct promise_type<std::monostate> final
  : public promise_type_base<std::monostate> {
  using Base = promise_type_base<std::monostate>;

  using Base::wait_promise_;

  promise_type() {
    auto h = std::coroutine_handle<promise_type>::from_promise(
        *this );
    wait_promise_.state()->set_coro(
        base::unique_coro<promise_type>( h ) );
  }

  ~promise_type() noexcept {
    auto h = std::coroutine_handle<promise_type>::from_promise(
        *this );
    remove_cpp_coroutine_if_queued( h );
  }

  void return_void() {
    wait_promise_.set_value( std::monostate{} );
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

namespace std {

template<typename T, typename... Args>
struct coroutine_traits<::rn::wait<T>, Args...> {
  using promise_type = ::rn::detail::promise_type<T>;
};

} // namespace std
