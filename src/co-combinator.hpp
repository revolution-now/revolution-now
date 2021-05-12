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

/****************************************************************
** any
*****************************************************************/
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

/****************************************************************
** all
*****************************************************************/
// FIXME: add unit tests for this.
waitable<> all( std::vector<waitable<>> ws );

waitable<> all( waitable<>&& w );
waitable<> all( waitable<>&& w1, waitable<>&& w2 );
waitable<> all( waitable<>&& w1, waitable<>&& w2,
                waitable<>&& w3 );

/****************************************************************
** first
*****************************************************************/
// FIXME: clang seems to have trouble with function templates
// that are coroutines, so we wrap it in a struct.
struct First {
  // Run the waitables ws in parallel, then return the result of
  // the first one that finishes.
  template<typename... Ts>
  waitable<base::variant<Ts...>> operator()(
      waitable<Ts>... ws ) const {
    waitable_promise<base::variant<Ts...>> wp;
    ( ws.shared_state()->add_callback( [wp]( Ts const& o ) {
      // First one wins.
      wp.set_value_emplace_if_not_set( o );
    } ),
      ... );
    // !! Need to co_await instead of just returning the waitable
    // because we need to keep the waitables alive.
    co_return co_await wp.waitable();
  }
};

inline constexpr First first{};

/****************************************************************
** with_cancel
*****************************************************************/
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

/****************************************************************
** background
*****************************************************************/
// FIXME: clang seems to have trouble with function templates
// that are coroutines, so we wrap it in a struct.
struct WithBackground {
  // Run the waitable w in parallel with the background task,
  // until w becomes ready, at which point return w's value. It
  // is inconsequential whether the background task finishes
  // early or not. If background is still running when w fin-
  // ishes, it will naturally be cancelled as it will go out of
  // scope.
  template<typename T>
  waitable<T> operator()( waitable<T> w,
                          waitable<>  background ) const {
    waitable_promise<T> wp;
    // Need to do w first so that if both are ready already then
    // w will take precedence and return its value.
    w.shared_state()->add_callback( [wp]( T const& o ) {
      wp.set_value_emplace_if_not_set( o );
    } );
    // !! Need to co_await instead of just returning the waitable
    // because we need to keep the waitables alive.
    co_return co_await wp.waitable();
  }
};

inline constexpr WithBackground background{};

/****************************************************************
** repeat
*****************************************************************/
waitable<> repeat(
    base::unique_func<waitable<>() const> coroutine );

/****************************************************************
** latch
*****************************************************************/
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

/****************************************************************
** ticker
*****************************************************************/
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

/****************************************************************
** stream
*****************************************************************/
template<typename T>
struct stream {
  waitable<T> next() {
    T res = co_await p.waitable();
    p     = {};
    update();
    co_return res;
  }

  void send( T const& t ) {
    q.push( t );
    update();
  }

  void send( T&& t ) {
    q.push_emplace( std::move( t ) );
    update();
  }

  bool ready() const { return p.has_value(); }

  void reset() {
    *this = {};
    // Not necessary, but for consistency.
    update();
  }

  stream()                = default;
  stream( stream const& ) = delete;
  stream& operator=( stream const& ) = delete;
  stream( stream&& )                 = default;
  stream& operator=( stream&& ) = default;

private:
  void update() {
    if( !p.has_value() && !q.empty() ) {
      p.set_value_emplace( std::move( *q.front() ) );
      q.pop();
    }
  }

  waitable_promise<T> p;
  flat_queue<T>       q;
};

/****************************************************************
** finite_stream
*****************************************************************/
template<typename T>
struct finite_stream {
  waitable<maybe<T>> next() {
    if( ended ) co_return nothing;
    maybe<T> res = co_await s.next();
    if( !res ) {
      ended = true;
      s.reset();
      co_return nothing;
    }
    co_return res;
  }

  void send( T const& t ) { s.send( t ); }
  void send( T&& t ) { s.send( std::move( t ) ); }
  void finish() { s.send( nothing ); }

  void reset() { *this = {}; }

private:
  bool             ended = false;
  stream<maybe<T>> s;
};

/****************************************************************
** detect_suspend
*****************************************************************/
template<typename T>
struct ResultWithSuspend {
  T    result;
  bool suspended;
};

struct DetectSuspend {
  // Wrap a waitable in this in order to detect whether it sus-
  // pends in the process of computing its result.
  template<typename T>
  waitable<ResultWithSuspend<T>> operator()(
      waitable<T>&& w ) const {
    ResultWithSuspend<T> res;
    res.suspended = !w.ready();
    res.result    = co_await std::move( w );
    co_return res;
  }
};

inline constexpr DetectSuspend detect_suspend{};

} // namespace rn::co
