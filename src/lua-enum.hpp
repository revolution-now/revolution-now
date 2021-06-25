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

// Rnl
#include "rnl/helper/enum.hpp"

// luapp
#include "luapp/ext.hpp"

#define LUA_ENUM_DECL( what )                               \
  base::maybe<e_##what> lua_get( ::lua::cthread L, int idx, \
                                 ::lua::tag<e_##what> );    \
  void lua_push( ::lua::cthread L, e_##what val );

#define LUA_ENUM( what )                                        \
  base::maybe<e_##what> lua_get( ::lua::cthread L, int idx,     \
                                 ::lua::tag<e_##what> ) {       \
    base::maybe<int> m = ::lua::get<int>( L, idx );             \
    if( !m ) return base::nothing;                              \
    return enum_traits<e_##what>::from_integral( *m );          \
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
    for( e_##what val : enum_traits<e_##what>::values )         \
      e[name][enum_traits<e_##what>::value_name( val )] = val;  \
  };                                                            \
  }
