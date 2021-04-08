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

namespace detail {

// The PromiseT is the type (inside the waitable) that will be
// returned by the coroutine that is awaiting on this awaitable.
// The type T is the type that this awaitable will yield.
template<typename PromiseT, typename T = std::monostate>
struct awaitable {
  waitable<T> w_;
  awaitable( waitable<T> w ) : w_( w ) {}
  bool await_ready() noexcept { return w_.ready(); }
  void await_suspend( coro::coroutine_handle<> h ) noexcept {
    DCHECK( w_.shared_state() );
    auto& coro_promise =
        coro::coroutine_handle<PromiseT>::from_address(
            h.address() )
            .promise()
            .waitable_promise_;
    coro_promise.shared_state()->set_cancel(
        [ss = w_.shared_state().get()] { ss->cancel(); } );
    w_.shared_state()->add_callback(
        [this, h = unique_coro( h )]( T const& ) mutable {
          this->w_.shared_state()->set_cancel( [h = h.get()] {
            destroy_queued_coroutine_handler( h );
          } );
          queue_coroutine_handle( std::move( h ) );
        } );
  }
  T await_resume() noexcept {
    // Need to remove the cancel function here since it is set to
    // delete this coroutine handle that is now being resumed.
    // This will cause a problem if the shared_state referred to
    // by w_ is being held by someone else and then they cancel
    // it (this can happen in combinator functions).
    w_.shared_state()->set_cancel();
    return w_.get();
  }
};

waitable<> await_transform_impl( FrameCount frame_count );
waitable<> await_transform_impl( std::chrono::milliseconds ms );

// The point of this is that it's not a template, so it can hold
// all the common stuff that does not depend on the type T para-
// meter of the promise type.
struct promise_type_base_base {
  auto initial_suspend() const { return base::suspend_never{}; }
  auto final_suspend() const noexcept {
    return base::suspend_never{};
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

  rn::waitable_promise<T> waitable_promise_{};
};

template<typename T>
struct promise_type final : public promise_type_base<T> {
  using Base = promise_type_base<T>;

  using Base::waitable_promise_;

  void return_value( T const& val ) {
    waitable_promise_.set_value( val );
  }
  void return_value( T&& val ) {
    waitable_promise_.set_value( std::move( val ) );
  }

  static auto await_transform( FrameCount frame_count ) {
    return awaitable<promise_type, std::monostate>(
        await_transform_impl( frame_count ) );
  }

  static auto await_transform( std::chrono::milliseconds ms ) {
    return awaitable<promise_type, std::monostate>(
        await_transform_impl( ms ) );
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

  void return_void() {
    waitable_promise_.set_value( std::monostate{} );
  }

  static auto await_transform( FrameCount frame_count ) {
    return awaitable<promise_type, std::monostate>(
        await_transform_impl( frame_count ) );
  }

  static auto await_transform( std::chrono::milliseconds ms ) {
    return awaitable<promise_type, std::monostate>(
        await_transform_impl( ms ) );
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
