/****************************************************************
**co-combinator.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-24.
*
* Description: Combinators for waitables.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "error.hpp"
#include "flat-queue.hpp"
#include "maybe.hpp"
#include "waitable-coro.hpp"

// base
#include "base/unique-func.hpp"

// C++ standard library
#include <vector>

namespace rn::co {

// Returns a waitable that will be ready when (and as soon as)
// the first waitable becomes ready. Since this function takes
// ownership of all of the waitables, they will be gone when this
// function becomes ready, and thus any that are not ready will
// be cancelled.
waitable<> any( std::vector<waitable<>> ws );

waitable<> any( waitable<>&& w );
waitable<> any( waitable<>&& w1, waitable<>&& w2 );
waitable<> any( waitable<>&& w1, waitable<>&& w2,
                waitable<>&& w3 );

// FIXME: clang seems to have trouble with function templates
// that are coroutines, so we wrap it in a struct.
struct WithCancel {
  // Run the waitable w in parallel with canceller. If canceller
  // becomes ready first then w is cancelled and `nothing` is re-
  // turned. If w becomes ready first then canceller is cancelled
  // and w's value is returned.
  template<typename T>
  waitable<maybe<T>> operator()( waitable<T> w,
                                 waitable<>  canceller ) const {
    waitable_promise<maybe<T>> wp;
    // Need to do w first so that if both are ready already then
    // w will take precedence and return its value.
    w.shared_state()->add_callback( [wp]( T const& o ) {
      wp.set_value_emplace_if_not_set( o );
    } );
    canceller.shared_state()->add_callback(
        [wp]( std::monostate const& ) {
          wp.set_value_emplace_if_not_set();
        } );
    // !! Need to co_await instead of just returning the waitable
    // because we need to keep the waitables alive.
    co_return co_await wp.waitable();
  }
};

inline constexpr WithCancel with_cancel{};

waitable<> repeat(
    base::unique_func<waitable<>() const> coroutine );

// A latch is meant to be a singlton object (per use-case) that
// can be awaited on by multiple things. When it is set, all
// awaiters will be resumed, and it will remain set until reset.
struct latch {
  void set() { p.set_value_emplace_if_not_set(); }
  void reset() { p = {}; }

  waitable<> waitable() const { return p.waitable(); }

  using Awaitable =
      detail::awaitable<detail::promise_type<std::monostate>>;

  Awaitable operator co_await() const noexcept {
    return Awaitable{ waitable() };
  }

  waitable_promise<> p;
};

// A ticker is meant to be a singlton object (per use-case) that
// can be awaited on by multiple things. When it is ticked, all
// the awaiters will be resumed, and it will be reset, so anyone
// who then starts newly awaiting on it will have to wait until
// the next tick.
struct ticker {
  void tick() {
    p.set_value_emplace();
    p = {};
  }

  waitable<> wait() const { return p.waitable(); }

private:
  waitable_promise<> p;
};

// Not sure if this supports multiple waiters... probably best
// just to stick to one.
template<typename T>
struct stream {
  waitable<T> next() {
    update();
    T res = co_await p.waitable();
    p     = {};
    co_return res;
  }

  void send( T const& t ) { q.push( t ); }
  void send( T&& t ) { q.push_emplace( std::move( t ) ); }

  void update() {
    if( !p.has_value() && !q.empty() ) {
      p.set_value_emplace( std::move( *q.front() ) );
      q.pop();
    }
  }

  void reset() { *this = {}; }

  stream()                = default;
  stream( stream const& ) = delete;
  stream& operator=( stream const& ) = delete;
  stream( stream&& )                 = default;
  stream& operator=( stream&& ) = default;

private:
  waitable_promise<T> p;
  flat_queue<T>       q;
};

} // namespace rn::co
