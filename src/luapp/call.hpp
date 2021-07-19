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
#include "ext.hpp"
#include "thread-status.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/error.hpp"
#include "base/function-ref.hpp"
#include "base/macros.hpp"
#include "base/maybe.hpp"

namespace lua {

/****************************************************************
** Interface: Function on stack, results on stack.
*****************************************************************/
// These functions will take the function object from the stack
// at position (-1) and will leave the results (if any) on the
// stack.

// Expects a function to be at the top of the stack and will call
// it with the given C++ args. The function will be run in unsafe
// mode, so any Lua errors will go uncaught. Otherwise, it re-
// turns the number of results that are on the stack.
template<Pushable... Args>
int call_lua_unsafe( cthread L, Args&&... args );

// Same as above but allows specifying nresults.
template<Pushable... Args>
void call_lua_unsafe_nresults( cthread L, int nresults,
                               Args&&... args );

// Expects a function to be at the top of the stack and will call
// it with the given C++ args. The function will be run in pro-
// tected mode, so any Lua errors will be caught and returned.
// Otherwise, it returns the number of results that are on the
// stack.
template<Pushable... Args>
lua_expect<int> call_lua_safe( cthread L,
                               Args&&... args ) noexcept;

// Same as above but allows specifying nresults.
template<Pushable... Args>
lua_valid call_lua_safe_nresults( cthread L, int nresults,
                                  Args&&... args ) noexcept;

// Calls lua_resume on L_toresume with the given arguments.
//
// There is no unsafe version; if an error happens, the thread
// will be reset and the error object will be returned (but not
// popped from the thread's stack).
//
// The first time this is called to initiate a coroutine, the
// function to be called must already be pushed onto the stack,
// then the push_args must push onto the L_toresume stack the
// initial arguments to the function. For subsequent calls to
// this function to resume the coroutine, push_args must push the
// arguments to be returned by coroutine.yield.
template<Pushable... Args>
lua_expect<resume_result> call_lua_resume_safe(
    cthread L_toresume, cthread L_from,
    Args&&... args ) noexcept;

/****************************************************************
** Interface: Function on stack, results converted+returned.
*****************************************************************/
// Expects a function to be at the top of the stack and will call
// it with the given C++ args. The result(s) will be popped off
// of the stack and converted to the requested C++ type and re-
// turned (unless R is void). The function will be run in unsafe
// mode, so any Lua errors will be left uncaught. Any errors en-
// countered while converting the Lua result values to the C++
// type will be thrown as a Lua error.
template<GettableOrVoid R = void, Pushable... Args>
R call_lua_unsafe_and_get( cthread L, Args&&... args );

// Same as above but, in the event of an error (either during ex-
// ecution or in converting the results to C++) the function will
// return the error. In other words, this function should never
// throw a Lua error or check-fail.
template<GettableOrVoid R = void, Pushable... Args>
error_type_for_return_type<R> call_lua_safe_and_get(
    cthread L, Args&&... args );

// Calls lua_resume on L_toresume with the given arguments. It
// will then resize the stack so that there are precisely the
// number of return values on the stack needed to extract a value
// of type `R', and will return that value along with the thread
// status. It will pop the results off of the stack as well. In
// the case of an error while running the coroutine, the thread
// will be reset and the error will be returned (but not popped
// off of the thread's stack).
template<GettableOrVoid R = void, Pushable... Args>
lua_expect<resume_result_with_value<R>>
call_lua_resume_safe_and_get( cthread L_toresume, cthread L_from,
                              Args&&... args );

/****************************************************************
** Implementation: Function on stack, results on stack.
*****************************************************************/
namespace internal {

// If nresults is `nothing` then it will use LUA_MULTRET.
lua_expect<int> call_lua_from_cpp(
    cthread L, base::maybe<int> nresults, bool safe,
    base::function_ref<void()> push_args );

lua_expect<resume_result> call_lua_resume_from_cpp(
    cthread L_toresume, cthread L_from,
    base::function_ref<void()> push_args );

} // namespace internal

template<Pushable... Args>
int call_lua_unsafe( cthread L, Args&&... args ) {
  // This result will always be present, since in unsafe mode we
  // will throw a Lua error (and thus not return) if anything
  // goes wrong.
  UNWRAP_CHECK(
      nresults,
      internal::call_lua_from_cpp(
          L, /*nresults=*/base::nothing, /*safe=*/false, [&] {
            ( push( L, std::forward<Args>( args ) ), ... );
          } ) );
  return nresults;
}

template<Pushable... Args>
void call_lua_unsafe_nresults( cthread L, int nresults,
                               Args&&... args ) {
  // This result will always be present, since in unsafe mode we
  // will throw a Lua error (and thus not return) if anything
  // goes wrong.
  CHECK( internal::call_lua_from_cpp(
      L, nresults, /*safe=*/false, [&] {
        ( push( L, std::forward<Args>( args ) ), ... );
      } ) );
}

template<Pushable... Args>
lua_expect<int> call_lua_safe( cthread L,
                               Args&&... args ) noexcept {
  return internal::call_lua_from_cpp(
      L, /*nresults=*/base::nothing, /*safe=*/true,
      [&] { ( push( L, std::forward<Args>( args ) ), ... ); } );
}

template<Pushable... Args>
lua_valid call_lua_safe_nresults( cthread L, int nresults,
                                  Args&&... args ) noexcept {
  HAS_VALUE_OR_RET( internal::call_lua_from_cpp(
      L, nresults, /*safe=*/true, [&] {
        ( push( L, std::forward<Args>( args ) ), ... );
      } ) );
  return base::valid;
}

template<Pushable... Args>
lua_expect<resume_result> call_lua_resume_safe(
    cthread L_toresume, cthread L_from,
    Args&&... args ) noexcept {
  return internal::call_lua_resume_from_cpp(
      L_toresume, L_from, [&] {
        ( push( L_toresume, std::forward<Args>( args ) ), ... );
      } );
}

/****************************************************************
** Implementation: Function on stack, results converted+returned.
*****************************************************************/
namespace internal {

void pop_call_results( cthread L, int n );

std::string lua_error_bad_return_values(
    cthread L, int nresults, std::string_view ret_type_name );

[[noreturn]] void throw_lua_error_bad_return_values(
    cthread L, int nresults, std::string_view ret_type_name );

// If nresults_returned < nresults_needed then this will push
// nils onto the stack to make up the difference. If
// nresults_returned > nresults_needed then values will be popped
// off of the stack. After this function, it is safe to pop
// nresults_needed from the stack.
void adjust_return_values( cthread L, int nresults_returned,
                           int nresults_needed );

lua_valid resetthread( cthread L );

} // namespace internal

template<GettableOrVoid R, Pushable... Args>
R call_lua_unsafe_and_get( cthread L, Args&&... args ) {
  static constexpr int nresults = nvalues_for<R>();
  // We need to set nresults here in order to ensure that pre-
  // cisely that many return values are pushed onto the stack.
  // Otherwise, Lua could return fewer than that, and our `get`
  // method (if it consumes multiple stack values) would run the
  // risk of consuming values that are already on the stack.
  call_lua_unsafe_nresults( L, nresults, FWD( args )... );
  if constexpr( !std::is_same_v<R, void> ) {
    // Should consume the nvalues starting at index (-1). Typi-
    // cally this will just be one value, but could be more.
    base::maybe<R> res = lua::get<R>( L, -1 );
    if( !res.has_value() )
      internal::throw_lua_error_bad_return_values(
          L, nresults, base::demangled_typename<R>() );
    internal::pop_call_results( L, nresults );
    return std::move( *res );
  }
}

template<GettableOrVoid R, Pushable... Args>
error_type_for_return_type<R> call_lua_safe_and_get(
    cthread L, Args&&... args ) {
  static constexpr int nresults = nvalues_for<R>();
  // We need to set nresults here in order to ensure that pre-
  // cisely that many return values are pushed onto the stack.
  // Otherwise, Lua could return fewer than that, and our `get`
  // method (if it consumes multiple stack values) would run the
  // risk of consuming values that are already on the stack.
  HAS_VALUE_OR_RET(
      call_lua_safe_nresults( L, nresults, FWD( args )... ) );
  if constexpr( !std::is_same_v<R, void> ) {
    // Should consume the nvalues starting at index (-1). Typi-
    // cally this will just be one value, but could be more.
    base::maybe<R> res = lua::get<R>( L, -1 );
    if( !res.has_value() ) {
      std::string msg = internal::lua_error_bad_return_values(
          L, nresults, base::demangled_typename<R>() );
      internal::pop_call_results( L, nresults );
      return lua_unexpected<R>( std::move( msg ) );
    }
    internal::pop_call_results( L, nresults );
    return std::move( *res );
  } else {
    return base::valid;
  }
}

template<GettableOrVoid R, Pushable... Args>
lua_expect<resume_result_with_value<R>>
call_lua_resume_safe_and_get( cthread L_toresume, cthread L_from,
                              Args&&... args ) {
  static constexpr int      nresults_needed = nvalues_for<R>();
  lua_expect<resume_result> res =
      call_lua_resume_safe( L_toresume, L_from, FWD( args )... );
  HAS_VALUE_OR_RET( res );
  // This basically does what call/pcall do when we specify a
  // fixed nresults, which we can't do for some reason with
  // lua_resume, so we do it manually.
  internal::adjust_return_values(
      L_toresume, /*nresults_returned=*/res->nresults,
      /*nresults_needed=*/nresults_needed );

  if constexpr( !std::is_same_v<R, void> ) {
    // Should consume the nvalues starting at index (-1). Typi-
    // cally this will just be one value, but could be more.
    base::maybe<R> ret_val = lua::get<R>( L_toresume, -1 );
    if( !ret_val.has_value() ) {
      std::string msg = internal::lua_error_bad_return_values(
          L_toresume, nresults_needed,
          base::demangled_typename<R>() );
      internal::pop_call_results( L_toresume, nresults_needed );
      // Here we need to reset the thread because it will not
      // have already been reset because the lua_resume actually
      // succeeded, it is just the conversion of the return value
      // that failed. Then to be consistent, we reset the thread
      // because we reset it when there is an error while running
      // the coroutine.
      (void)internal::resetthread( L_toresume );
      return lua_unexpected<resume_result_with_value<R>>(
          std::move( msg ) );
    }
    internal::pop_call_results( L_toresume, nresults_needed );
    return resume_result_with_value<R>{
        .status = res->status, .value = std::move( *ret_val ) };
  } else {
    return resume_result_with_value<void>{ .status =
                                               res->status };
  }
}

} // namespace lua
