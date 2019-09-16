/****************************************************************
**lua.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-13.
*
* Description: Interface to lua.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"
#include "sol.hpp"

// base-util
#include "base-util/macros.hpp"
#include "base-util/pp.hpp"
#include "base-util/type-map.hpp"

// C++ standard library
#include <string>
#include <variant>

namespace rn::lua {

/****************************************************************
** Run Lua Scripts
*****************************************************************/
namespace detail {
using LuaRetMap = TypeMap<   //
    KV<void, std::monostate> //
    >;
}

template<typename Ret>
expect<Get<detail::LuaRetMap, Ret, Ret>> run(
    Str const& script );

template<>
expect<std::monostate> run<void>( Str const& script );

template<>
expect<Str> run<Str>( Str const& script );

// sol::state& state();

/****************************************************************
** Registration
*****************************************************************/
#define LUA_FN( ... ) PP_N_OR_MORE_ARGS_3( LUA_FN, __VA_ARGS__ )

#define LUA_FN_STARTUP( ns, name )             \
  STARTUP() {                                  \
    lua::register_fn( []( sol::state& st ) {   \
      st[#ns].get_or_create<sol::table>();     \
      st[#ns][#name] = lua_fn_##ns##_##name{}; \
    } );                                       \
  }

#define LUA_FN_SINGLE( ns, name, ret_type ) \
  struct lua_fn_##ns##_##name {             \
    ret_type operator()() const;            \
  };                                        \
  LUA_FN_STARTUP( ns, name )                \
  ret_type lua_fn_##ns##_##name::operator()() const

#define LUA_FN_MULTI( ns, name, ret_type, ... ) \
  struct lua_fn_##ns##_##name {                 \
    ret_type operator()( __VA_ARGS__ ) const;   \
  };                                            \
  LUA_FN_STARTUP( ns, name )                    \
  ret_type lua_fn_##ns##_##name::operator()( __VA_ARGS__ ) const

using RegistrationFn_t = std::function<void( sol::state& )>;

void register_fn( RegistrationFn_t fn );

/****************************************************************
** Utilites
*****************************************************************/
Vec<Str> format_lua_error_msg( Str const& msg );

/****************************************************************
** Testing
*****************************************************************/
void test_lua();

} // namespace rn::lua
