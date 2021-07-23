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
#include "co-lua-scheduler.hpp"
#include "lua.hpp"
#include "waitable.hpp"

// luapp
#include "luapp/any.hpp"
#include "luapp/cast.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/ext-std.hpp"
#include "luapp/ext-userdata.hpp"
#include "luapp/rfunction.hpp"
#include "luapp/rthread.hpp"
#include "luapp/state.hpp"

// base
#include "base/scope-exit.hpp"

// C++ standard library
#include <stdexcept>

namespace lua {

template<Pushable T>
struct type_traits<::rn::waitable<T>>
  : TraitsForModel<::rn::waitable<T>,
                   e_userdata_ownership_model::owned_by_lua> {
  static void register_usertype( state& st ) {
    using W = ::rn::waitable<T>;
    auto ut = st.usertype.create<W>();

    ut["cancel"] = &W::cancel;
    ut["ready"]  = &W::ready;
    ut["get"]    = []( W& w ) { return w.get(); };

    ut["error"] = []( W& w ) -> base::maybe<std::string> {
      if( !w.has_exception() ) return base::nothing;
      return base::rethrow_and_get_msg( w.exception() );
    };

    ut["set_resume"] = []( W& w, lua::rthread coro ) {
      w.shared_state()->add_callback( [coro]( T const& ) {
        rn::queue_lua_coroutine( coro );
      } );
      w.shared_state()->set_exception_callback(
          [coro]( std::exception_ptr ) {
            rn::queue_lua_coroutine( coro );
          } );
    };
    ut[lua::metatable_key]["__close"] =
        []( W& w, lua::any /*error_object*/ ) { w.cancel(); };
  }

  inline static int registration = [] {
    constexpr static rn::LuaRegistrationFnSig* reg_addr =
        &register_usertype;
    rn::register_lua_fn( &reg_addr );
    return 0;
  }();

  // The purpose of this static_assert is to force `registration`
  // to be ODR-used, otherwise it will not be instantiated (since
  // this is a template class) and then the usertype won't be
  // registered. This trick was taken from:
  //
  //   stackoverflow.com/questions/6420985/
  //       how-to-force-a-static-member-to-be-initialized
  static_assert( &registration == &registration );
};

} // namespace lua

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
