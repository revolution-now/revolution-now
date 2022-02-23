/****************************************************************
**lua-enum.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-24.
*
* Description: For exposing enums to Lua.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// luapp
#include "luapp/ext.hpp"

// refl
#include "refl/query-enum.hpp"

#define LUA_ENUM_DECL( what )                               \
  base::maybe<e_##what> lua_get( ::lua::cthread L, int idx, \
                                 ::lua::tag<e_##what> );    \
  void lua_push( ::lua::cthread L, e_##what val );

#define LUA_ENUM( what )                                        \
  base::maybe<e_##what> lua_get( ::lua::cthread L, int idx,     \
                                 ::lua::tag<e_##what> ) {       \
    base::maybe<int> m = ::lua::get<int>( L, idx );             \
    if( !m ) return base::nothing;                              \
    return refl::enum_from_integral<e_##what>( *m );            \
  }                                                             \
                                                                \
  void lua_push( ::lua::cthread L, e_##what val ) {             \
    ::lua::push( L, static_cast<int>( val ) );                  \
  }                                                             \
                                                                \
  namespace {                                                   \
  LUA_STARTUP( ::lua::state& st ) {                             \
    ::lua::table::create_or_get( st[::rn::lua_module_name__] ); \
    auto e = ::lua::table::create_or_get( st["e"] );            \
    constexpr std::string_view name = #what;                    \
    CHECK( e[name] == lua::nil,                                 \
           "symbol named `{}` has already been registered.",    \
           name );                                              \
    e[name] = st.table.create();                                \
    for( e_##what val : refl::enum_values<e_##what> )           \
      e[name][refl::enum_value_name<e_##what>( val )] = val;    \
  };                                                            \
  }
