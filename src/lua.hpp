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
ND expect<Get<detail::LuaRetMap, Ret, Ret>> run(
    Str const& script );

template<>
ND expect<std::monostate> run<void>( Str const& script );

template<>
ND expect<Str> run<Str>( Str const& script );

void load_modules();
void run_startup();

/****************************************************************
** Registration
*****************************************************************/
using RegistrationFn_t = std::function<void( sol::state& )>;

void register_fn( RegistrationFn_t fn );

/****************************************************************
** Functions
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

/****************************************************************
** Enums
*****************************************************************/
template<typename Enum, size_t... Indexes>
void register_enum_impl( sol::state& st, std::string_view name,
                         std::index_sequence<Indexes...> ) {
  auto e = st["e"].get_or_create<sol::table>();
  CHECK( e[name] == sol::lua_nil,
         "symbol named `{}` has already been registered.",
         name );
  e.new_enum<Enum>(
      name,
      std::initializer_list<std::pair<std::string_view, Enum>>{
          std::pair{
              Enum::_from_index_nothrow( Indexes )->_to_string(),
              *Enum::_from_index_nothrow( Indexes )}...} );
}

template<typename Enum>
void register_enum( sol::state& st, std::string_view name ) {
  return register_enum_impl<Enum>(
      st, name, std::make_index_sequence<Enum::_size()>() );
}

#define LUA_ENUM( what )                                        \
  STARTUP() {                                                   \
    lua::register_fn( []( sol::state& st ) {                    \
      lua::register_enum<e_##what>( st, #what );                \
      st["e"][#what "_from_string"] = []( char const* name ) {  \
        auto maybe_val =                                        \
            e_##what::_from_string_nothrow( name );             \
        CHECK(                                                  \
            maybe_val,                                          \
            "enum value `{}` is not a member of the enum `{}`", \
            name, #what );                                      \
        return *maybe_val;                                      \
      };                                                        \
    } );                                                        \
  }

/****************************************************************
** Utilites
*****************************************************************/
Vec<Str> format_lua_error_msg( Str const& msg );

/****************************************************************
** Testing
*****************************************************************/
void test_lua();

} // namespace rn::lua
