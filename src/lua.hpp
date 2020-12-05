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
#include "cc-specific.hpp"
#include "coord.hpp"
#include "errors.hpp"
#include "id.hpp"
#include "sol.hpp"
#include "typed-int.hpp"

// base-util
#include "base-util/macros.hpp"
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

using LuaRetMap = TypeMap< //
    KV<void, xp_success_t> //
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
    return nothing;
  auto xp_T = sol_obj_convert<T>( o );
  if( !xp_T.has_value() ) //
    return nothing;
  return Opt<T>{ *xp_T };
}

template<typename Ret>
expect<Ret> lua_script( std::string_view script ) {
  auto result = global_state().safe_script(
      script, sol::script_pass_on_error );
  if( !result.valid() ) {
    sol::error err = result;
    return UNEXPECTED( err.what() );
  }
  if constexpr( std::is_same_v<Ret, xp_success_t> )
    return xp_success_t{};
  else if constexpr( base::is_maybe_v<Ret> )
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
// This is only needed in modules that include only LUA_STARTUP
// macros (which themselves won't register the module name),
// though it shouldn't hurt to include it.
#define LUA_MODULE()                                    \
  LUA_STARTUP( sol::state& st ) {                       \
    st[lua::module_name__].get_or_create<sol::table>(); \
  };

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
              magic_enum::enum_name(
                  *magic_enum::enum_cast<Enum>( Indexes ) ),
              *magic_enum::enum_cast<Enum>( Indexes ) }... } );
  // FIXME: add in a "from string" method that allows converting
  // from a string to the enum value, then re-enable the associ-
  // ated unit test case that is now commented out.
}

template<typename Enum>
void register_enum( sol::state& st, std::string_view name ) {
  return register_enum_impl<Enum>(
      st, name,
      std::make_index_sequence<
          magic_enum::enum_count<Enum>()>() );
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
          magic_enum::enum_cast<::rn::e_##what>( name );       \
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
    return name{ val };                                       \
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
        { { "-", "_" } } );
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
LUA_TYPED_INT( ::rn::X );
LUA_TYPED_INT( ::rn::Y );
LUA_TYPED_INT( ::rn::W );
LUA_TYPED_INT( ::rn::H );

LUA_TYPED_INT( ::rn::UnitId );
LUA_TYPED_INT( ::rn::ColonyId );

/****************************************************************
** Coord
*****************************************************************/
namespace rn {

template<typename Handler>
inline bool sol_lua_check( sol::types<::rn::Coord>, lua_State* L,
                           int index, Handler&& handler,
                           sol::stack::record& tracking ) {
  int  absolute_index = lua_absindex( L, index );
  bool success        = sol::stack::check<sol::table>(
      L, absolute_index, handler );
  tracking.use( 1 );
  if( !success ) return false;
  auto table = sol::stack::get<sol::table>( L, absolute_index );
  success    = ( table["x"].get_type() == sol::type::number ) &&
            ( table["y"].get_type() == sol::type::number );
  if( !success ) return false;
  return success;
}

inline ::rn::Coord sol_lua_get( sol::types<::rn::Coord>,
                                lua_State* L, int index,
                                sol::stack::record& tracking ) {
  int  absolute_index = lua_absindex( L, index );
  auto table = sol::stack::get<sol::table>( L, absolute_index );
  ::rn::Coord coord{ table["x"].get<X>(), table["y"].get<Y>() };
  tracking.use( 1 );
  return coord;
}

inline int sol_lua_push( sol::types<::rn::Coord>, lua_State* L,
                         ::rn::Coord const& coord ) {
  sol::state_view st( L );

  auto table = sol::lua_value( st, {} ).as<sol::table>();
  table["x"] = coord.x;
  table["y"] = coord.y;
  // Delegate to lua factory function. We could do this in C++
  // but the native lua factory function will add in some meta
  // methods and it's nice not to have to duplicate that here.
  CHECK( st["Coord"] != sol::lua_nil,
         "The native Lua Coord factory function must be defined "
         "first before converting from C++ Coord to Lua." );
  table      = st["Coord"]( table );
  int amount = sol::stack::push( L, table );

  /* amount will be 1: int pushes 1 item. */
  return amount;
}

} // namespace rn

/****************************************************************
** maybe
*****************************************************************/
// The `maybe<T>` type is treated specially when it comes to in-
// teracting with Lua. As one would expect, when a maybe<T> is
// converted to Lua, it is given a value of `nil` when it is
// `nothing` and a value of T when it is a T. However, when con-
// verting from Lua to C++, any Lua value will be accepted; if it
// happens to be a T then it will convert to a `maybe<T>` with
// value. If it is `nil` then it will convert to `nothing`. How-
// ever, if it is any other type, it will not be an error, in-
// stead it will be converted to an empty `maybe<T>` type (noth-
// ing). So in other words, any Lua value of any type can be de-
// serialized to a `maybe<T>` for any type T!
//
// The reason for this behavior is that it allows the user to re-
// trieve a value (say, of type U) from Lua either as a U di-
// rectly (if they are sure that it is a U) or as a `maybe<U>` if
// they are unsure it is a `U` and the conversion will succeed
// from Lua's point of view, giving the user a (possibly empty)
// `maybe<U>` value that they can then check for success. This
// means that the user cannot distinguish between a Lua value of
// `nil` and an "incorrect type" when converting a `maybe<U>`
// type from Lua to C++, That is just a tradeoff that is made for
// the benefit of using `maybe<U>` as a failsafe.
//
// Actually, sol2 does the same thing (automatically) with
// std::optional, so it is only consistent to do the same with
// base::maybe. The thing is, we have to implement this ourselves
// since sol2 doesn't know what a base::maybe is. That is the
// purpose of the three functions below.
namespace base {

template<typename Handler, typename T>
inline bool sol_lua_check( sol::types<::rn::maybe<T>>,
                           lua_State* L, int index,
                           Handler&&           handler,
                           sol::stack::record& tracking ) {
  // I think we just have to check that there is any object at
  // all on the stack, then return true, since, as stated above,
  // any Lua value of any type can be converted to a `maybe<T>`
  // for any type T.
  int  absolute_index = lua_absindex( L, index );
  bool success        = sol::stack::check<sol::object>(
      L, absolute_index, handler );
  tracking.use( 1 );
  return success;
}

template<typename T>
inline ::rn::maybe<T> sol_lua_get(
    sol::types<::rn::maybe<T>>, lua_State* L, int index,
    sol::stack::record& tracking ) {
  static_assert( !std::is_reference_v<T> );
  int absolute_index = lua_absindex( L, index );
  tracking.use( 1 );
  ::rn::maybe<T> m;
  auto o = sol::stack::get<sol::object>( L, absolute_index );
  sol::lua_value v( L, o );
  if( !v.is<T>() ) return m;
  m = v.as<T>();
  return m;
}

template<typename T>
inline int sol_lua_push( sol::types<::rn::maybe<T>>,
                         lua_State*            L,
                         ::rn::maybe<T> const& m ) {
  sol::lua_value v = sol::lua_nil;
  if( m.has_value() ) v = *m;
  int amount = sol::stack::push( L, v );
  return amount;
}

} // namespace base
