/****************************************************************
**lua-root.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-12.
*
* Description: TODO [FILL ME IN]
*
*****************************************************************/
// #include "lua-root.hpp"

// ss
#include "error.hpp"
#include "ext.hpp"
#include "root.rds.hpp"

// luapp
#include "luapp/any.hpp"
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/ext-std.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

// traverse
#include "traverse/ext-std.hpp"
#include "traverse/ext.hpp"
#include "traverse/type-ext.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/scope-exit.hpp"
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

/****************************************************************
** TypeTraverse Specializations.
*****************************************************************/
namespace trv {

template<template<typename> typename O, typename T>
struct TypeTraverse<O, ::base::maybe<T>>
  : traverse_base<TypeTraverse<O, T>, O<::base::maybe<T>>> {};

// TODO: may need this later.
// template<template<typename> typename O, typename... Ts>
// struct TypeTraverse<O, std::tuple<Ts...>>
//   : traverse_base<TypeTraverse<O, Ts>...,
//   O<std::tuple<Ts...>>> {
// };

template<template<typename> typename O, typename... Ts>
struct TypeTraverse<O, std::tuple<::refl::StructField<Ts>...>>
  : traverse_base<TypeTraverse<
        O, typename ::refl::StructField<Ts>::type>...> {};

template<template<typename> typename O, refl::ReflectedStruct S>
struct TypeTraverse<O, S>
  : traverse_base<
        TypeTraverse<O,
                     std::remove_const_t<
                         decltype( ::refl::traits<S>::fields )>>,
        O<S>> {};

TRV_TYPE_TRAVERSE( std::map, K, V );
TRV_TYPE_TRAVERSE( ::refl::enum_map, K, V );

} // namespace trv

/****************************************************************
** Lua Traits
*****************************************************************/
namespace lua {

template<refl::ReflectedStruct S>
struct type_traits<S>
  : TraitsForModel<S, e_userdata_ownership_model::owned_by_cpp> {
};

// Ensure this one doesn't get type traits from the reflected
// struct version above, since Coord uses ADL for push/get.
template<>
struct type_traits<::rn::Coord> {};

template<Stackable K, Stackable V>
struct type_traits<std::map<K, V>>
  : TraitsForModel<std::map<K, V>,
                   e_userdata_ownership_model::owned_by_cpp> {};

template<Stackable K, Stackable V>
struct type_traits<::refl::enum_map<K, V>>
  : TraitsForModel<::refl::enum_map<K, V>,
                   e_userdata_ownership_model::owned_by_cpp> {};

} // namespace lua

/****************************************************************
** Lua Usertypes
*****************************************************************/
namespace lua {
namespace {

template<typename T>
requires std::is_scalar_v<T>
void define_usertype_for( lua::state&, tag<T> ) {}

template<typename T>
void define_usertype_for( lua::state&, tag<base::maybe<T>> ) {}

void define_usertype_for( lua::state&, tag<::rn::Coord> ) {}

template<typename K, typename V>
void define_usertype_for( lua::state& st, tag<map<K, V>> ) {
  using U = map<K, V>;
  auto u  = st.usertype.create<U>();

  u["size"] = []( U& o ) -> int { return o.size(); };

  table mt           = u[metatable_key];
  auto const __index = mt["__index"].template as<rfunction>();

  u[metatable_key]["__index"] =
      [__index]( U& o, any const key ) -> base::maybe<any> {
    if( auto const member = __index( o, key ); member != nil )
      return member.template as<any>();
    auto const maybe_key = safe_as<K>( key );
    if( !maybe_key.has_value() ) return base::nothing;
    auto const iter = o.find( *maybe_key );
    if( iter == o.end() ) return base::nothing;
    auto const L = __index.this_cthread();
    return as<any>( L, iter->second );
  };

  u[metatable_key]["__newindex"] = []( U& o, K const& key,
                                       V val ) {
    o[key] = std::move( val );
  };
}

template<typename K, typename V>
void define_usertype_for( lua::state& st,
                          tag<::refl::enum_map<K, V>> ) {
  using U = ::refl::enum_map<K, V>;
  auto u  = st.usertype.create<U>();

  table mt           = u[metatable_key];
  auto const __index = mt["__index"].template as<rfunction>();

  u[metatable_key]["__index"] =
      [__index]( U& o, any const key ) -> base::maybe<any> {
    if( auto const member = __index( o, key ); member != nil )
      return member.template as<any>();
    auto const maybe_key = safe_as<K>( key );
    if( !maybe_key.has_value() ) return base::nothing;
    auto& val    = o[*maybe_key];
    auto const L = __index.this_cthread();
    return as<any>( L, val );
  };

  u[metatable_key]["__newindex"] = []( U& o, K const& key,
                                       V val ) {
    o[key] = std::move( val );
  };
}

template<refl::ReflectedStruct S>
void define_usertype_for( lua::state& st, tag<S> ) {
  auto u = st.usertype.create<S>();
  trv::traverse( refl::traits<S>::fields,
                 [&]( auto& field, string_view const ) {
                   // Here the `name` passed into this lambda is
                   // e.g. "<0>", "<1>", because we are tra-
                   // versing the tuple. So we need field.name to
                   // get the real field name.
                   u[field.name] = field.accessor;
                 } );
}

} // namespace
} // namespace lua

/****************************************************************
** RegisterLuaCppType
*****************************************************************/
namespace rn {

template<typename T>
struct RegisterLuaType {
  using type = T;

  // Ensure that we're never pushable in two different ways.
  static_assert( lua::PushableViaTraits<T&> !=
                 lua::PushableViaAdl<T> );

  inline static string const kTypeName = base::str_replace_all(
      base::demangled_typename<T>(), { { " >", ">" } } );

  inline static auto const register_fn = +[]( lua::state& st ) {
    fmt::println( "define_usertype_for: {}", kTypeName );
    ::lua::define_usertype_for( st, ::lua::tag<T>{} );
  };

  inline static auto const p_register_fn     = +register_fn;
  inline static auto* const* p_p_register_fn = &p_register_fn;

  inline static int _ = [] {
    fmt::println( "registering lua typename: {}", kTypeName );
    ::lua::detail::register_lua_fn( p_p_register_fn );
    return 0;
  }();
  ODR_USE_MEMBER( _ );
};

TRV_RUN_TYPE_TRAVERSE( RegisterLuaType, RootState2 );

/****************************************************************
** Linker.
*****************************************************************/
void linker_dont_discard_module_ss_lua_root();
void linker_dont_discard_module_ss_lua_root() {}

} // namespace rn
