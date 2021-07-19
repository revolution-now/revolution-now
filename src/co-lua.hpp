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
#include "base/source-loc.hpp"

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

// We're putting this as a struct because it seems tricky to have
// a function template with variadic arguments that also can take
// a default SourceLoc parameter.
template<lua::Gettable R = lua::any>
struct lua_waitable {
  base::SourceLoc loc_;
  lua_waitable(
      base::SourceLoc loc = base::SourceLoc::current() )
    : loc_( loc ) {}

  template<LuaAwaitable T, lua::Pushable... Args>
  waitable<R> operator()( T&& o, Args&&... args ) const {
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
                 [&]( R res ) {
                   p.set_value( std::move( res ) );
                 },
                 /*set_error=*/
                 [&]( std::string msg ) {
                   std::string err_line = fmt::format(
                       "{}:{}: {}", loc_.file_name(),
                       loc_.line(), msg );
                   p.template set_exception_emplace<
                       lua_error_exception>(
                       std::move( err_line ) );
                 },
                 f, FWD( args )... );

    // Need to keep the SCOPE_EXIT alive while we wait.
    co_return co_await p.waitable();
  }
};

void linker_dont_discard_module_co_lua();

} // namespace rn
