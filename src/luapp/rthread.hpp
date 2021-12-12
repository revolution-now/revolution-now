/****************************************************************
**rthread.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII holder for registry references to Lua
*              threads.
*
*****************************************************************/
#pragma once

// luapp
#include "any.hpp"
#include "call.hpp"
#include "thread-status.hpp"

// base
#include "base/fmt.hpp"

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** rthread
*****************************************************************/
struct rthread : public any {
  using Base = any;

  rthread( lua::cthread L, int ref );

  friend base::maybe<rthread> lua_get( lua::cthread L, int idx,
                                       tag<rthread> );

  // In an rthread, the particular `L' held represents the
  // thread. This is unlike other objects where the L is only
  // held for access to the global state.
  lua::cthread cthread() const noexcept { return L; }

  bool is_main() const noexcept;

  // As of Lua 5.4.4, this will reset the error state as well.
  lua_valid        resetthread() const noexcept;
  thread_status    status() const noexcept;
  coroutine_status coro_status() const noexcept;

  template<typename R = void, typename... Args>
  [[nodiscard]] R resume( Args&&... args ) {
    lua::cthread L = this->cthread();

    lua_expect<resume_result_with_value<R>> res =
        call_lua_resume_safe_and_get<R>( L, FWD( args )... );
    if( !res )
      throw_lua_error( L,
                       "error while resuming Lua coroutine: {}",
                       res.error() );
    if constexpr( !std::is_same_v<R, void> )
      return std::move( res->value );
  }

  template<typename R = void, typename... Args>
  error_type_for_return_type<R> resume_safe( Args&&... args ) {
    lua::cthread L = this->cthread();

    lua_expect<resume_result_with_value<R>> res =
        call_lua_resume_safe_and_get<R>( L, FWD( args )... );
    if( !res ) return res.error();
    if constexpr( !std::is_same_v<R, void> )
      return std::move( res->value );
    else
      return base::valid;
  }
};

static_assert( Stackable<rthread> );

// For when we want to push onto the stack of another thread.
template<Pushable T>
int push( rthread th, T&& o ) {
  return lua::push( th.cthread(), std::forward<T>( o ) );
}

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::rthread );
