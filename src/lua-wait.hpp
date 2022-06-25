/****************************************************************
**lua-wait.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-23.
*
* Description: Lua userdata type traits for wait<T>.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "co-lua-scheduler.hpp"
#include "wait.hpp"

// luapp
#include "luapp/any.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/ext-userdata.hpp"
#include "luapp/ext.hpp"
#include "luapp/state.hpp"
#include "luapp/usertype.hpp"

namespace lua {

namespace detail {
using LuaRegistrationFnSig = void( lua::state& );
void register_lua_fn( LuaRegistrationFnSig* const* fn );
}

template<Pushable T>
struct type_traits<::rn::wait<T>>
  : TraitsForModel<::rn::wait<T>,
                   e_userdata_ownership_model::owned_by_lua> {
  static void register_usertype( state& st ) {
    using W = ::rn::wait<T>;
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
    constexpr static detail::LuaRegistrationFnSig* reg_addr =
        &register_usertype;
    lua::detail::register_lua_fn( &reg_addr );
    return 0;
  }();

  // The purpose of this static_assert is to force `registration`
  // to be ODR-used, otherwise it will not be instantiated (since
  // this is a template class) and then the usertype won't be
  // registered. This trick was taken from:
  //
  //   stackoverflow.com/questions/6420985/
  //       how-to-force-a-static-member-to-be-initialized
  //
  // The following also seems to work:
  //
  //   static constexpr auto const force_register =
  //           &registration;
  //
  static_assert( &registration == &registration );
};

} // namespace lua
