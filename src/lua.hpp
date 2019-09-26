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
#include "base-util/optional.hpp"
#include "base-util/pp.hpp"
#include "base-util/type-map.hpp"

// Abseil
#include "absl/strings/str_replace.h"

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
  return static_cast<Ret>( o.as<intermediate_t>() );
}

template<typename T>
expect<Opt<T>> sol_opt_convert( sol::object const& o ) {
  if( o.is<sol::lua_nil_t>() ) //
    return std::nullopt;
  XP_OR_RETURN( xp_T, sol_obj_convert<T>( o ) );
  return Opt<T>{*xp_T};
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
  else if constexpr( util::is_optional_v<Ret> )
    return sol_opt_convert<typename Ret::value_type>(
        result.get<sol::object>() );
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
void run_startup_routines();
void load_modules();
void run_startup_main();

void reload();

/****************************************************************
** Registration
*****************************************************************/
using RegistrationFnSig = void( sol::state& );

void register_fn( std::string_view    module_name,
                  RegistrationFnSig** fn );

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
#define LUA_STARTUP( st )                                       \
  struct STRING_JOIN( register_, __LINE__ ) {                   \
    STRING_JOIN( register_, __LINE__ )() {                      \
      rn::lua::register_fn( rn::lua::module_name__, &init_fn ); \
    }                                                           \
    static rn::lua::RegistrationFnSig* init_fn;                 \
  } STRING_JOIN( obj, __LINE__ );                               \
  rn::lua::RegistrationFnSig* STRING_JOIN(                      \
      register_, __LINE__ )::init_fn = []( st )

/****************************************************************
** Registration: Functions
*****************************************************************/
#define LUA_FN( ... ) PP_N_OR_MORE_ARGS_2( LUA_FN, __VA_ARGS__ )

#define LUA_FN_STARTUP( name )                          \
  LUA_STARTUP( sol::state& st ) {                       \
    st[lua::module_name__].get_or_create<sol::table>(); \
    st[lua::module_name__][#name] = lua_fn_##name{};    \
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
** Registration: Enums
*****************************************************************/
namespace detail {

template<typename Enum, size_t... Indexes>
void register_enum_impl( sol::state& st, std::string_view name,
                         std::index_sequence<Indexes...> ) {
  auto e = st["e"].get_or_create<sol::table>();
  CHECK( e[name] == sol::lua_nil,
         "symbol named `{}` has already been registered.",
         name );
  // Somehow making it read-only prevents iteration over the
  // values (sol bug?). In any case, we will manually make it
  // readonly later with lua code.
  e.new_enum<Enum, /*read_only=*/false>(
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

/*
template<typename Enum>
sol::variadic_results mt_pairs_enum( sol::table tbl ) {
  sol::variadic_results res;
  sol::state_view       st( tbl.lua_state() );
  sol::lua_value        value( st );
  value = [tbl]( sol::table, Enum k ) {
    sol::variadic_results res;
    sol::state_view       st( tbl.lua_state() );
    sol::lua_value        value( st );

    auto next =
        util::find_subsequent_and_cycle( values<Enum>, k );
    if( next == values<Enum>[0] ) {
      res.push_back( sol::lua_nil );
    } else {
      value = next;
      res.push_back( value.as<sol::object>() );
      value = "enum value";
      res.push_back( value.as<sol::object>() );
    }
    return res;
  };
  res.push_back( value.as<sol::function>() );
  res.push_back( tbl );
  value = values<Enum>[0];
  res.push_back( value.as<sol::object>() );
  return res;
}
*/

} // namespace detail

#define LUA_ENUM( what )                                       \
  LUA_STARTUP( sol::state& st ) {                              \
    ::rn::lua::detail::register_enum<::rn::e_##what>( st,      \
                                                      #what ); \
    st[::rn::lua::module_name__].get_or_create<sol::table>();  \
    st["e"][#what "_from_string"] = []( char const* name ) {   \
      auto maybe_val =                                         \
          ::rn::e_##what::_from_string_nothrow( name );        \
      CHECK(                                                   \
          maybe_val,                                           \
          "enum value `{}` is not a member of the enum `{}`",  \
          name, #what );                                       \
      return *maybe_val;                                       \
    };                                                         \
  };

/****************************************************************
** Registration: Typed Int
*****************************************************************/
#define LUA_TYPED_INT( name )                                 \
  template<typename Handler>                                  \
  inline bool sol_lua_check( sol::types<name>, lua_State* L,  \
                             int index, Handler&& handler,    \
                             sol::stack::record& tracking ) { \
    int  absolute_index = lua_absindex( L, index );           \
    bool success        = sol::stack::check<double>(          \
        L, absolute_index, handler );                  \
    tracking.use( 1 ); /* use one stack slot. */              \
    return success;                                           \
  }                                                           \
  inline name sol_lua_get( sol::types<name>, lua_State* L,    \
                           int                 index,         \
                           sol::stack::record& tracking ) {   \
    int absolute_index = lua_absindex( L, index );            \
    int val =                                                 \
        int( sol::stack::get<double>( L, absolute_index ) );  \
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

namespace {
auto module_name__ =
#ifdef LUA_MODULE_NAME_OVERRIDE
    LUA_MODULE_NAME_OVERRIDE;
#else
    // Used by above macros when registering functions.
    absl::StrReplaceAll(
        fs::path( __BASE_FILE__ ).filename().stem().string(),
        {{"-", "_"}} );
#endif
} // namespace

/****************************************************************
** Testing
*****************************************************************/
void test_lua();

void reset_state();

} // namespace rn::lua

/****************************************************************
** Customizations
*****************************************************************/
#include "lua-ext.hpp"
