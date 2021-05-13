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
#include "co-registry.hpp"
#include "frame-count.hpp"
#include "waitable.hpp"

// base
#include "base/co-compat.hpp"

namespace rn {

namespace co {
// By co_await'ing on this you can abort a coroutine.
struct abort_t {};
inline constexpr abort_t abort{};
} // namespace co

namespace detail {

// The PromiseT is the type (inside the waitable) that will be
// returned by the coroutine that is awaiting on this awaitable.
// The type T is the type that this awaitable will yield.
template<typename PromiseT, typename T = std::monostate>
struct awaitable {
  waitable<T> w_;
  awaitable( waitable<T> w ) : w_( std::move( w ) ) {}
  // The below functions, in addition to retrieving the result of
  // the waitable, also handle linking. There are two types of
  // linking that have to happen: both of them point from up-
  // stream (leaf) to downstream (root). The first is to add a
  // link (callback in this case) from the new waitable's
  // shared_state to the coroutine that is currently running so
  // that when that waitable is fullfilled it will schedule this
  // coroutine for resumption. The second kind of linking is to
  // link the new waitable's shared state to the shared state as-
  // sociated with the promise in this coroutine so that a leaf
  // (the most upstream code) can abort the coroutine and have it
  // propagate back through the coroutine call stack to the
  // root-most one that will catch it.
  bool await_ready() noexcept { return w_.ready(); }
  void await_suspend( coro::coroutine_handle<> h ) noexcept {
    w_.shared_state()->add_callback(
        [h]( T const& ) { queue_coroutine_handle( h ); } );
    PromiseT& coro_p =
        coro::coroutine_handle<PromiseT>::from_address(
            h.address() )
            .promise();
    w_.shared_state()->link_aborter(
        *coro_p.waitable_promise_.shared_state() );
    // !! The above statement may cause the whole coroutine stack
    // (including this one) to be destroyed, if we are linking to
    // a waitable that has already been aborted. That's why we do
    // it here, because at this point in the await_suspend
    // method, the coroutine is considered to be suspended, so we
    // can delete the coroutine if we want.
  }
  T await_resume() noexcept {
    // No need to unlink the callback here, just the aborter.
    // This should be a no-op if the waitable has not been
    // linked, as would be the case if it was already ready and
    // await_suspend was not called.
    w_.shared_state()->unlink_aborter();
    return w_.get();
  }
};

waitable<> await_transform_impl( FrameCount frame_count );
waitable<std::chrono::microseconds> await_transform_impl(
    std::chrono::microseconds us );

// The point of this is that it's not a template, so it can hold
// all the common stuff that does not depend on the type T para-
// meter of the promise type.
struct promise_type_base_base {
  auto initial_suspend() const { return base::suspend_never{}; }
  auto final_suspend() const noexcept {
    return base::suspend_always{};
  }

  void unhandled_exception() { SHOULD_NOT_BE_HERE; }
};

template<typename T>
struct promise_type_base : public promise_type_base_base {
  using Base = promise_type_base_base;

  using Base::Base;

  auto get_return_object() {
    return waitable_promise_.waitable();
  }

  auto await_transform( co::abort_t ) {
    struct suspend_always_and_abort {
      waitable_promise<T>* wp;
      bool await_ready() noexcept { return false; }
      void await_suspend( coro::coroutine_handle<> ) noexcept {
        // A this point the coroutine is suspended, so the line
        // below can, theoretically, cause the destruction of
        // this coroutine and it should be fine.
        wp->abort();
        // !! don't access `this`, as this coroutine might be
        // gone.
      }
      void await_resume() noexcept {}
    };
    return suspend_always_and_abort{ &waitable_promise_ };
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

  using Base::await_transform;

  static auto await_transform( FrameCount frame_count ) {
    return awaitable<promise_type, std::monostate>(
        await_transform_impl( frame_count ) );
  }

  static auto await_transform( std::chrono::microseconds us ) {
    return awaitable<promise_type, std::chrono::microseconds>(
        await_transform_impl( us ) );
  }

  template<typename Waitable>
  static auto await_transform( Waitable&& w ) {
    return awaitable<promise_type, typename std::remove_cvref_t<
                                       Waitable>::value_type>(
        std::forward<Waitable>( w ) );
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

  using Base::await_transform;

  static auto await_transform( FrameCount frame_count ) {
    return awaitable<promise_type, std::monostate>(
        await_transform_impl( frame_count ) );
  }

  static auto await_transform( std::chrono::microseconds us ) {
    return awaitable<promise_type, std::chrono::microseconds>(
        await_transform_impl( us ) );
  }

  template<typename Waitable>
  static auto await_transform( Waitable&& w ) {
    return awaitable<promise_type, typename std::remove_cvref_t<
                                       Waitable>::value_type>(
        std::forward<Waitable>( w ) );
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
