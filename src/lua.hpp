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
#include "util.hpp"

// base-util
#include "base-util/macros.hpp"
#include "base-util/pp.hpp"
#include "base-util/type-map.hpp"

// C++ standard library
#include <string>
#include <variant>

namespace rn::lua {

/****************************************************************
** Lua (Sol2) State
*****************************************************************/
sol::state& global_state();

/****************************************************************
** Run Lua Scripts
*****************************************************************/
namespace detail {

using LuaRetMap = TypeMap<   //
    KV<void, std::monostate> //
    >;

using IntermediateCppTypeMap = TypeMap< //
    KV<int, double>                     //
    >;

template<typename Ret>
expect<Ret> sol_obj_convert( sol::object const& o ) {
  using intermediate_t = Get<IntermediateCppTypeMap, Ret, Ret>;
  if( !o.is<intermediate_t>() ) {
    std::string via =
        std::is_same_v<Ret, intermediate_t>
            ? ""
            : ( std::string( " (via `" ) +
                demangled_typename<intermediate_t>() + "`)" );
    return UNEXPECTED(
        fmt::format( "expected type `{}`{} but got `{}`.",
                     demangled_typename<Ret>(), via,
                     sol::type_name( global_state().lua_state(),
                                     o.get_type() ) ) );
  }
  auto intermediate_res = o.as<intermediate_t>();
  return static_cast<Ret>( intermediate_res );
}

template<typename Ret>
expect<Ret> lua_script( std::string_view script ) {
  auto result = global_state().safe_script(
      script, sol::script_pass_on_error );
  if( !result.valid() ) {
    sol::error err = result;
    return UNEXPECTED( err.what() );
  }
  if constexpr( std::is_same_v<Ret, std::monostate> )
    return std::monostate{};
  else
    return sol_obj_convert<Ret>( result.get<sol::object>() );
}

} // namespace detail

template<typename Ret>
ND auto run( Str const& script ) {
  return detail::lua_script<Get<detail::LuaRetMap, Ret, Ret>>(
      script );
}

/****************************************************************
** Lua Modules
*****************************************************************/
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

#define LUA_FN_STARTUP( ns, name )                 \
  STARTUP() {                                      \
    ::rn::lua::register_fn( []( sol::state& st ) { \
      st[#ns].get_or_create<sol::table>();         \
      st[#ns][#name] = lua_fn_##ns##_##name{};     \
    } );                                           \
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
    ::rn::lua::register_fn( []( sol::state& st ) {              \
      ::rn::lua::register_enum<::rn::e_##what>( st, #what );    \
      st["e"][#what "_from_string"] = []( char const* name ) {  \
        auto maybe_val =                                        \
            ::rn::e_##what::_from_string_nothrow( name );       \
        CHECK(                                                  \
            maybe_val,                                          \
            "enum value `{}` is not a member of the enum `{}`", \
            name, #what );                                      \
        return *maybe_val;                                      \
      };                                                        \
    } );                                                        \
  }

/****************************************************************
** Typed Int
*****************************************************************/
#define LUA_TYPED_INT( name )                                 \
  template<typename Handler>                                  \
  inline bool sol_lua_check( sol::types<name>, lua_State* L,  \
                             int index, Handler&& handler,    \
                             sol::stack::record& tracking ) { \
    int  absolute_index = lua_absindex( L, index );           \
    bool success =                                            \
        sol::stack::check<int>( L, absolute_index, handler ); \
    tracking.use( 1 ); /* use one stack slot. */              \
    return success;                                           \
  }                                                           \
  inline name sol_lua_get( sol::types<name>, lua_State* L,    \
                           int                 index,         \
                           sol::stack::record& tracking ) {   \
    int absolute_index = lua_absindex( L, index );            \
    int val = sol::stack::get<int>( L, absolute_index );      \
    tracking.use( 1 ); /* use one stack slot. */              \
    return name{val};                                         \
  }                                                           \
  inline int sol_lua_push( sol::types<name>, lua_State* L,    \
                           name const& n ) {                  \
    int amount = sol::stack::push( L, n._ );                  \
    /* amount will be 1: int pushes 1 item. */                \
    return amount;                                            \
  }

/****************************************************************
** Utilites
*****************************************************************/
Vec<Str> format_lua_error_msg( Str const& msg );

/****************************************************************
** Testing
*****************************************************************/
void test_lua();

void reset_state();

} // namespace rn::lua
