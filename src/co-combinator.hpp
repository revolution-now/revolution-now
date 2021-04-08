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
#include "maybe.hpp"
#include "waitable.hpp"

// base
#include "base/unique-func.hpp"

// C++ standard library
#include <vector>

namespace rn::co {

// **************************************************************
// NOTE: All functions in this module must return waitables that
// are properly cancellable.
// **************************************************************

// Given a waitable w that is composed in some way from some con-
// stituent waitables ws, this function will add the cancella-
// bility property which is required for it to work properly in
// this framework where all waitables should support proper can-
// cellability (meaning that when they are cancelled they should
// propagate the cancellation up the coroutine chain).
void chain_cancellation( waitable<>&                    w,
                         std::vector<waitable<>> const& ws );

// Returns a waitable that will be ready when (and as soon as)
// the first waitable becomes ready. All of the others will be
// cancelled, although some of the others may be ready as well.
// If the vector is empty, it will be in a ready state.
waitable<> any( std::vector<waitable<>> ws );

template<typename... Waitables>
waitable<> any( Waitables&&... ws ) {
  return any( { std::forward<Waitables>( ws )... } );
}

// Returns a waitable that will be ready when all of the ws are
// ready.
waitable<> all( std::vector<waitable<>> ws );

template<typename... Waitables>
waitable<> all( Waitables&&... ws ) {
  return all( std::vector{ std::forward<Waitables>( ws )... } );
}

// While `w` is running, run `background`. Then wait until `w`
// has finished, then cancel `background`.
//
// Implementation: The implementation of this is tricky; we can't
// just do the following:
//
//   T res = co_await w;
//   background.cancel();
//
// because then the resulting waitable would not be correctly
// cancellable, because if we cancel during the first step, the
// `background` would not get cancelled.
template<typename T>
waitable<T> until_do( waitable<T> what, waitable<> background ) {
  waitable_promise<> wp;
  maybe<T>           res;
  // Need to pass w as a parameter and not a capture because cap-
  // tures are not preserved in the coroutine frame and we are
  // executing the lambda immediately, so any captures would go
  // away too soon.
  waitable<> run = []( waitable_promise<> wp, waitable<T> what,
                       maybe<T>& dst ) -> waitable<> {
    dst.emplace( co_await what );
    wp.finish();
  }( wp, what, res );
  waitable<> w = wp.waitable();
  chain_cancellation( w, { background, run } );
  co_await w;
  CHECK( res );
  background.cancel();
  if constexpr( !std::is_same_v<T, std::monostate> )
    co_return *res;
}

waitable<> repeat(
    base::unique_func<waitable<>() const> coroutine );

waitable<> repeat(
    base::unique_func<waitable<>() const> coroutine, int n );

/****************************************************************
** Monadic functions.
*****************************************************************/
// fmap: apply function to value inside of waitable, returning
// another waitable.
template<typename Func, typename T>
waitable<std::invoke_result_t<Func, T>> fmap( Func&&      func,
                                              waitable<T> w ) {
  co_return std::forward<Func>( func )( co_await w );
}

// bind: apply function to results of waitable, which returns a
// new waitable.
template<typename Func, typename T>
waitable<typename std::invoke_result_t<Func, T>::value_type>
bind( waitable<T> w, Func&& func ) {
  co_return co_await std::forward<Func>( func )( co_await w );
}

// join: collapse nested waitables.
template<typename T>
waitable<T> join( waitable<waitable<T>> w ) {
  co_return co_await co_await w;
}

// invoke: apply function to results of waitables.
template<typename Func, typename T1, typename T2>
waitable<std::invoke_result_t<Func, T1, T2>> invoke(
    Func&& func, waitable<T1> w1, waitable<T2> w2 ) {
  co_return std::forward<Func>( func )( co_await w1,
                                        co_await w2 );
}

} // namespace rn::co
