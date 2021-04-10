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

waitable<> repeat(
    base::unique_func<waitable<>() const> coroutine );

// A ticker is meant to be a singlton object (per use-case) that
// can be awaited on by multiple things. When it is ticked, all
// the awaiters will be resumed, and it will be reset.
//
// Example:
//
//   co::ticker my_ticker;
//
//   Consumer:
//      co_await my_ticker.wait();
//
//   Producer:
//     my_ticker.tick();
//
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
