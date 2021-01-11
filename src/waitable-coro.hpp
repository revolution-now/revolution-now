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
auto operator co_await( waitable<T> const& sf ) {
  struct awaitable {
    waitable<T> sf_;
    bool        await_ready() noexcept { return sf_.ready(); }
    void await_suspend( coro::coroutine_handle<> h ) noexcept {
      register_coroutine_handle( h );
      sf_.shared_state()->add_callback(
          [h]( T const& ) { queue_coroutine_handle( h ); } );
    }
    T await_resume() noexcept { return sf_.get_and_reset(); }
  };
  return awaitable{ sf };
}

} // namespace rn

namespace CORO_NS {

template<typename T, typename... Args>
struct coroutine_traits<::rn::waitable<T>, Args...> {
  struct promise_type {
    promise_type() = default;

    void set( T const& val ) { s_promise_.set_value( val ); }
    void set( T&& val ) {
      s_promise_.set_value( std::move( val ) );
    }

    auto initial_suspend() const {
      return base::suspend_never{};
    }
    auto final_suspend() const noexcept {
      return base::suspend_never{};
    }

    void return_value( T const& val ) {
      s_promise_.set_value( val );
    }
    void return_value( T&& val ) {
      s_promise_.set_value( std::move( val ) );
    }

    auto get_return_object() {
      return s_promise_.get_waitable();
    }

    void unhandled_exception() { SHOULD_NOT_BE_HERE; }

    rn::waitable_promise<T> s_promise_{};
  };
};

} // namespace CORO_NS
