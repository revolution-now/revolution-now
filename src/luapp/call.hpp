/****************************************************************
**call.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-15.
*
* Description: Functions for calling Lua function via C++.
*
*****************************************************************/
#pragma once

// luapp
#include "cthread.hpp"
#include "error.hpp"

// base
#include "base/error.hpp"
#include "base/function-ref.hpp"
#include "base/maybe.hpp"

namespace lua {

namespace detail {

// If nresults is `nothing` then it will use LUA_MULTRET.
lua_expect<int> call_lua_from_cpp(
    cthread L, base::maybe<int> nresults, bool safe,
    base::function_ref<void()> push_args );

} // namespace detail

// Expects a function to be at the top of the stack and will call
// it with the given C++ args. The function will be run in unsafe
// mode, so any Lua errors will be thrown as such. Otherwise, it
// returns the number of results that are on the stack.
template<typename... Args>
int call_lua_unsafe( cthread L, Args&&... args ) {
  // This result will always be present, since in unsafe mode we
  // will throw a Lua error (and thus not return) if anything
  // goes wrong.
  UNWRAP_CHECK(
      nresults,
      detail::call_lua_from_cpp(
          L, /*nresults=*/base::nothing, /*safe=*/false, [&] {
            ( push( L, std::forward<Args>( args ) ), ... );
          } ) );
  return nresults;
}

// Same as above but allows specifying nresults.
template<typename... Args>
void call_lua_unsafe_nresults( cthread L, int nresults,
                               Args&&... args ) {
  // This result will always be present, since in unsafe mode we
  // will throw a Lua error (and thus not return) if anything
  // goes wrong.
  CHECK( detail::call_lua_from_cpp(
      L, nresults, /*safe=*/false, [&] {
        ( push( L, std::forward<Args>( args ) ), ... );
      } ) );
}

// Expects a function to be at the top of the stack and will call
// it with the given C++ args. The function will be run in pro-
// tected mode, so any Lua errors will be caught and returned.
// Otherwise, it returns the number of results that are on the
// stack.
template<typename... Args>
lua_expect<int> call_lua_safe( cthread L,
                               Args&&... args ) noexcept {
  return detail::call_lua_from_cpp(
      L, /*nresults=*/base::nothing, /*safe=*/true,
      [&] { ( push( L, std::forward<Args>( args ) ), ... ); } );
}

// Same as above but allows specifying nresults.
template<typename... Args>
lua_valid call_lua_safe_nresults( cthread L, int nresults,
                                  Args&&... args ) noexcept {
  UNWRAP_CHECK( actual_nresults,
                detail::call_lua_from_cpp(
                    L, nresults, /*safe=*/true, [&] {
                      ( push( L, std::forward<Args>( args ) ),
                        ... );
                    } ) );
  CHECK_EQ( actual_nresults, nresults );
  return base::valid;
}

} // namespace lua
