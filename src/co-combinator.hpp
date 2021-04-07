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
#include "waitable.hpp"

// base
#include "base/unique-func.hpp"

// C++ standard library
#include <vector>

namespace rn::co {

// Returns a waitable that will be ready when at least one of the
// waitables becomes ready. The other waitables may still be run-
// ning. If the vector is empty it will be in a ready state.
waitable<> any( std::vector<waitable<>> ws );

template<typename... Waitables>
waitable<> any( Waitables&&... ws ) {
  return any( { std::forward<Waitables>( ws )... } );
}

// Same as above, except the unfinished waitables (if any) will
// be cancelled. For the vector version: if the vector is empty,
// it will be in a ready state.
waitable<> any_cancel( std::vector<waitable<>> ws );

template<typename... Waitables>
waitable<> any_cancel( Waitables&&... ws ) {
  return any_cancel( { std::forward<Waitables>( ws )... } );
}

// The `get_repeatable` will be called repeatedly to get and run
// a waitable. This will continue ad infinitum or until the
// `until_this_finishes` finishes at which case an currently run-
// ning waitable will be cancelled. The `until_this_finishes`
// just runs continuously and is never restarted.
waitable<> repeat_until_and_cancel(
    base::unique_func<waitable<>() const> get_repeatable,
    waitable<>                            until_this_finishes );

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
