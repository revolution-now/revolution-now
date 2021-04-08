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
#include "base/scope-exit.hpp"
#include "base/unique-func.hpp"

// C++ standard library
#include <vector>

namespace rn::co {

// **************************************************************
// NOTE: All functions in this module must return waitables that
// are properly cancellable.
// **************************************************************

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
// FIXME: we're putting this function in a struct to avoid a
// weird clang compiler crash. At some point try moving this out
// into a free function called until_do.
struct UntilDo {
  // Can't just do the following:
  //
  //   T res = co_await w;
  //   background.cancel();
  //
  // because then the resulting waitable would not be correctly
  // cancellable, because if we cancel during the first step, the
  // `background` would not get cancelled.
  template<typename T>
  waitable<T> operator()( waitable<T> what,
                          waitable<>  background ) const {
    SCOPE_EXIT( background.cancel() );
    // Can't do a `return` here because of the SCOPE_EXIT above.
    co_return co_await what;
  }

  waitable<> operator()( waitable<> what,
                         waitable<> background ) const;
};

inline constexpr auto until_do = UntilDo{};

waitable<> repeat(
    base::unique_func<waitable<>() const> coroutine );

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
