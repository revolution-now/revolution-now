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

// Rnl
#include "rnl/helper/enum.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/fs.hpp"

// base-util
#include "base-util/macros.hpp"
#include "base-util/pp.hpp"

// Abseil
// FIXME: get rid of this
#include "absl/strings/str_replace.h"

#define LUA_CHECK( st, a, ... )                          \
  {                                                      \
    if( !( a ) ) {                                       \
      st.error( fmt::format(                             \
          "{} is false. {}", #a,                         \
          fmt::format( FMT_SAFE( "" __VA_ARGS__ ) ) ) ); \
    }                                                    \
  }

namespace lua {

struct state;

}

namespace rn {

/****************************************************************
** Lua State
*****************************************************************/
lua::state& lua_global_state();

/****************************************************************
** Lua Modules
*****************************************************************/
void run_lua_startup_routines();
void load_lua_modules();
void run_lua_startup_main();

void lua_reload();

/****************************************************************
** Registration
*****************************************************************/
using LuaRegistrationFnSig = void( lua::state& );

void register_lua_fn( LuaRegistrationFnSig* const* fn );

/****************************************************************
** Registration: General
*****************************************************************/
// For startup code that just needs access to the lua state.

// In the constructor of this temp class we register a pointer to
// a function pointer so that we can work around initialization
// order issues. Specifically, the constructor may run before the
// `init_fn` static variable is initialized (due to the order in
// which they appear in the translation unit) and so we just
// capture the address of it instead of its value. This init_fn
// won't be dereferenced and called until long after all static
// initialization has completed.
#define LUA_STARTUP( st )                     \
  struct STRING_JOIN( register_, __LINE__ ) { \
    STRING_JOIN( register_, __LINE__ )() {    \
      rn::register_lua_fn( &init_fn );        \
    }                                         \
    static rn::LuaRegistrationFnSig* init_fn; \
  } STRING_JOIN( obj, __LINE__ );             \
  rn::LuaRegistrationFnSig* STRING_JOIN(      \
      register_, __LINE__ )::init_fn = []( st )

/****************************************************************
** Registration: Functions
*****************************************************************/
// This is only needed in modules that include only LUA_STARTUP
// macros (which themselves won't register the module name),
// though it shouldn't hurt to include it.
#define LUA_MODULE()                                          \
  LUA_STARTUP( lua::state& st ) {                             \
    ::lua::table::create_or_get( st[rn::lua_module_name__] ); \
  };

#define LUA_FN( ... ) PP_N_OR_MORE_ARGS_2( LUA_FN, __VA_ARGS__ )

#define LUA_FN_STARTUP( name )                                \
  LUA_STARTUP( lua::state& st ) {                             \
    ::lua::table::create_or_get( st[rn::lua_module_name__] ); \
    st[rn::lua_module_name__][#name] = lua_fn_##name{};       \
  };

#define LUA_FN_SINGLE( name, ret_type ) \
  struct lua_fn_##name {                \
    ret_type operator()() const;        \
  };                                    \
  LUA_FN_STARTUP( name )                \
  ret_type lua_fn_##name::operator()() const

#define LUA_FN_MULTI( name, ret_type, ... )   \
  struct lua_fn_##name {                      \
    ret_type operator()( __VA_ARGS__ ) const; \
  };                                          \
  LUA_FN_STARTUP( name )                      \
  ret_type lua_fn_##name::operator()( __VA_ARGS__ ) const

/****************************************************************
** Registration: Typed Int
*****************************************************************/
/****************************************************************
** Utilites
*****************************************************************/
std::vector<std::string> format_lua_error_msg(
    std::string const& msg );

namespace {
auto lua_module_name__ =
#ifdef LUA_MODULE_NAME_OVERRIDE
    LUA_MODULE_NAME_OVERRIDE;
#else
    // Used by above macros when registering functions.
    absl::StrReplaceAll(
        fs::path( __BASE_FILE__ ).filename().stem().string(),
        { { "-", "_" } } );
#endif
} // namespace

/****************************************************************
** Testing
*****************************************************************/
void reset_state();

} // namespace rn
