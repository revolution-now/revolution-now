/****************************************************************
**co-lua.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-01.
*
* Description: Helpers for calling C++ coroutines from Lua.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "waitable.hpp"

// luapp
#include "luapp/any.hpp"
#include "luapp/cast.hpp"
#include "luapp/ext-userdata.hpp"
#include "luapp/rfunction.hpp"
#include "luapp/rthread.hpp"

// base
#include "base/scope-exit.hpp"

// C++ standard library
#include <stdexcept>

namespace lua {

LUA_USERDATA_TRAITS( rn::waitable<lua::any>, owned_by_lua ){};

}

namespace rn {

namespace internal {

lua::rthread create_runner_coro( lua::cthread L );

void cleanup_coro( lua::rthread coro );

} // namespace internal

struct lua_error_exception : std::runtime_error {
  lua_error_exception( std::string msg )
    : std::runtime_error( std::move( msg ) ) {}
};

template<typename T>
concept LuaAwaitable =
    lua::Castable<T, lua::rfunction> && lua::HasCthread<T>;

template<typename T>
concept GettableOrMonostate =
    lua::Gettable<T> || std::same_as<std::monostate, T>;

// We're putting this as a struct as a workaround because clang
// somehow doesn't like function templates that are coroutines.
template<GettableOrMonostate R>
struct LuaWaitable {
  template<LuaAwaitable T, lua::Pushable... Args>
  waitable<R> operator()( T&& o, Args&&... args ) const {
    static constexpr bool is_mono =
        std::is_same_v<R, std::monostate>;
    lua::rthread coro =
        internal::create_runner_coro( o.this_cthread() );
    // Ensure that all to-be-closed variables get closed and the
    // coroutine gets de-queued if an error happens.
    SCOPE_EXIT( internal::cleanup_coro( coro ) );

    lua::rfunction      f = lua::cast<lua::rfunction>( o );
    waitable_promise<R> p;
    // Start running it; if it finishes right away then the re-
    // sulting waitable should be ready right away. If not, then
    // we will suspend on the co_await below. In other words, we
    // don't care about the return values of the coro.resume
    // calls (ei- ther the first or subsequent) because the value
    // is returned to us in the waitable.
    coro.resume( /*set_result=*/
                 [&]( lua::any res ) {
                   if constexpr( is_mono )
                     p.set_value_emplace();
                   else
                     p.set_value( lua::cast<R>( res ) );
                 },
                 /*set_error=*/
                 [&]( std::string msg ) {
                   p.template set_exception_emplace<
                       lua_error_exception>( std::move( msg ) );
                 },
                 f, FWD( args )... );

    // Need to keep the SCOPE_EXIT alive while we wait.
    if constexpr( is_mono )
      co_await p.waitable();
    else
      co_return co_await p.waitable();
  }
};

template<GettableOrMonostate R = std::monostate>
inline constexpr LuaWaitable<R> lua_waitable{};

void linker_dont_discard_module_co_lua();

} // namespace rn
