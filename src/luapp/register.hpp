/****************************************************************
**register.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-24.
*
* Description: Machinery for registering userdata initialization.
*
*****************************************************************/
#pragma once

// luapp
#include "state.hpp"

// base
#include "base/fs.hpp" // FIXME
#include "base/string.hpp"

// base-util
#include "base-util/macros.hpp"
#include "base-util/pp.hpp"

namespace lua {

namespace detail {

using LuaRegistrationFnSig = void( lua::state& );

void register_lua_fn( LuaRegistrationFnSig* const* fn );

}

std::vector<detail::LuaRegistrationFnSig* const*>&
registration_functions();

/****************************************************************
** Macros
*****************************************************************/
// For startup code that just needs access to the lua state. In
// the constructor of this temp class we register a pointer to a
// function pointer so that we can work around initialization
// order issues. Specifically, the constructor may run before the
// `init_fn` static variable is initialized (due to the order in
// which they appear in the translation unit) and so we just cap-
// ture the address of it instead of its value. This init_fn
// won't be dereferenced and called until long after all static
// initialization has completed.
#define LUA_STARTUP( st )                                \
  struct STRING_JOIN( register_, __LINE__ ) {            \
    STRING_JOIN( register_, __LINE__ )() {               \
      ::lua::detail::register_lua_fn( &init_fn );        \
    }                                                    \
    static ::lua::detail::LuaRegistrationFnSig* init_fn; \
  } STRING_JOIN( obj, __LINE__ );                        \
  ::lua::detail::LuaRegistrationFnSig* STRING_JOIN(      \
      register_, __LINE__ )::init_fn = []( st )

// `name` is supposed to be a string.
#define LUA_AUTO_FN( name, func )              \
  LUA_STARTUP( lua::state& st ) {              \
    ::lua::table::create_or_get(               \
        st[::lua::lua_module_name__] );        \
    st[::lua::lua_module_name__][name] = func; \
  };

#define LUA_FN( ... ) PP_N_OR_MORE_ARGS_2( LUA_FN, __VA_ARGS__ )

#define LUA_FN_STARTUP( name )                                 \
  LUA_STARTUP( lua::state& st ) {                              \
    ::lua::table::create_or_get(                               \
        st[::lua::lua_module_name__] );                        \
    st[::lua::lua_module_name__][#name] = lua_fn_##name{ st }; \
  };

#define LUA_FN_SINGLE( name, ret_type ) \
  struct lua_fn_##name {                \
    ::lua::state& st;                   \
    ret_type      operator()() const;   \
  };                                    \
  LUA_FN_STARTUP( name )                \
  ret_type lua_fn_##name::operator()() const

#define LUA_FN_MULTI( name, ret_type, ... )        \
  struct lua_fn_##name {                           \
    ::lua::state& st;                              \
    ret_type      operator()( __VA_ARGS__ ) const; \
  };                                               \
  LUA_FN_STARTUP( name )                           \
  ret_type lua_fn_##name::operator()( __VA_ARGS__ ) const

/****************************************************************
** Lua Module Name
*****************************************************************/
namespace {
auto lua_module_name__ =
#ifdef LUA_MODULE_NAME_OVERRIDE
    LUA_MODULE_NAME_OVERRIDE;
#else
    // Used by above macros when registering functions.
    base::str_replace_all(
        fs::path( __BASE_FILE__ ).filename().stem().string(),
        { { "-", "_" } } );
#endif
} // namespace

} // namespace lua
