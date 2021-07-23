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
#include "luapp/ext-std.hpp"
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
    lua::rthread coro =
        internal::create_runner_coro( o.this_cthread() );
    // Ensure that all to-be-closed variables get closed and the
    // coroutine gets de-queued if an error happens.
    SCOPE_EXIT( internal::cleanup_coro( coro ) );

    waitable_promise<R> p;

    auto set_result = [&]( lua::any res ) {
      p.set_value( lua::cast<R>( res ) );
    };
    auto set_error = [&]( std::string msg ) {
      p.template set_exception_emplace<lua_error_exception>(
          std::move( msg ) );
    };

    // Start running it; if it finishes right away then the re-
    // sulting waitable should be ready right away. If not, then
    // we will suspend on the co_await below. In other words, we
    // don't care about the return values of the coro.resume
    // calls (either the first or subsequent) because the value
    // is returned to us in the waitable.
    //
    // Although this version of `resume` will propagate errors
    // (exceptions), that is not relevant here, because the func-
    // tion we are immediately calling (the "runner") uses pcallk
    // to call `o`, so all errors are caught and diverted into
    // the promise as stored exceptions. We could have used
    // resume_safe here, it doesn't make a difference, especially
    // because the return type is void and so we won't have any
    // errors converting the return type.
    coro.resume( std::move( set_result ), std::move( set_error ),
                 FWD( o ), FWD( args )... );

    // If an exception already happened before the first yield
    // point, then the waitable will have an exception in it, and
    // it will be dealt with by the co_await.
    //
    // Need to keep the SCOPE_EXIT alive while we wait.
    if constexpr( std::is_same_v<R, std::monostate> )
      co_await p.waitable();
    else
      co_return co_await p.waitable();
  }
};

template<GettableOrMonostate R = std::monostate>
inline constexpr LuaWaitable<R> lua_waitable{};

void linker_dont_discard_module_co_lua();

} // namespace rn
