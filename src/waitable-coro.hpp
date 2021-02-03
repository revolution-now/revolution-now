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
#include "waitable.hpp"

// base
#include "base/co-compat.hpp"

namespace rn {

template<typename T>
auto operator co_await( waitable<T> const& w ) {
  struct awaitable {
    waitable<T> w_;
    bool        await_ready() noexcept { return w_.ready(); }
    void await_suspend( coro::coroutine_handle<> h ) noexcept {
      register_coroutine_handle( h );
      w_.shared_state()->add_callback(
          [h]( T const& ) { queue_coroutine_handle( h ); } );
    }
    T await_resume() noexcept { return w_.get(); }
  };
  return awaitable{ w };
}

namespace detail {

template<typename T>
struct promise_type_base {
  promise_type_base() = default;

  void set( T const& val ) { s_promise_.set_value( val ); }
  void set( T&& val ) {
    s_promise_.set_value( std::move( val ) );
  }

  auto initial_suspend() const { return base::suspend_never{}; }
  auto final_suspend() const noexcept {
    return base::suspend_never{};
  }

  auto get_return_object() { return s_promise_.get_waitable(); }

  void unhandled_exception() { SHOULD_NOT_BE_HERE; }

  rn::waitable_promise<T> s_promise_{};
};

template<typename T>
struct promise_type : public promise_type_base<T> {
  using Base = promise_type_base<T>;

  using Base::s_promise_;

  void return_value( T const& val ) {
    s_promise_.set_value( val );
  }
  void return_value( T&& val ) {
    s_promise_.set_value( std::move( val ) );
  }
};

// Specialization for waitable<> that allows us to just use bare
// `co_return` statements instead of `co_return {}`.
template<>
struct promise_type<std::monostate>
  : public promise_type_base<std::monostate> {
  using Base = promise_type_base<std::monostate>;

  using Base::s_promise_;

  void return_void() {
    s_promise_.set_value( std::monostate{} );
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
