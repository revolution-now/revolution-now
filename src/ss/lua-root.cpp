/****************************************************************
**lua-root.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-12.
*
* Description: Exposes the entire ss structure to Lua.
*
*****************************************************************/
// #include "lua-root.hpp"

// ss
#include "root.rds.hpp"

// luapp
#include "luapp/any.hpp"
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/ext-std.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"
#include "luapp/usertype.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/query-struct.hpp"
#include "refl/to-str.hpp"

// traverse
#include "traverse/ext-std.hpp"
#include "traverse/ext.hpp"
#include "traverse/type-ext.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/error.hpp"
#include "base/odr.hpp"
#include "base/scope-exit.hpp"
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

/****************************************************************
** TypeTraverse Specializations.
*****************************************************************/
namespace trv {

TRV_TYPE_TRAVERSE( ::base::maybe, T );
TRV_TYPE_TRAVERSE( ::gfx::Matrix, T );
TRV_TYPE_TRAVERSE( ::refl::enum_map, K, V );
TRV_TYPE_TRAVERSE( ::rn::GenericUnitId );
TRV_TYPE_TRAVERSE( ::rn::MovementPoints );
TRV_TYPE_TRAVERSE( ::rn::UnitComposition );
TRV_TYPE_TRAVERSE( ::rn::UnitType );
TRV_TYPE_TRAVERSE( ::std::deque, T );
TRV_TYPE_TRAVERSE( ::std::map, K, V );
TRV_TYPE_TRAVERSE( ::std::string );
TRV_TYPE_TRAVERSE( ::std::unordered_map, K, V );
TRV_TYPE_TRAVERSE( ::std::vector, T );

// std::tuple of refl::StructFields
template<template<typename> typename O, typename... Ts>
struct TypeTraverse<O, std::tuple<::refl::StructField<Ts>...>> {
  using type = trv::list<typename O<
      typename ::refl::StructField<Ts>::type>::type...>;
};

// std::array
template<template<typename> typename O, typename T, size_t N>
struct TypeTraverse<O, std::array<T, N>> {
  using type = trv::list<typename O<T>::type>;
};

// ReflectedStruct
template<template<typename> typename O,
         ::refl::ReflectedStruct S>
struct TypeTraverse<O, S> {
  // NOTE: this is non-standard because we don't want to actually
  // traverse the tuple<StructField...>, instead we want to skip
  // straight to the underlying field types.
  using type = TypeTraverse<
      O, std::remove_const_t<
             decltype( ::refl::traits<S>::fields )>>::type;
};

// Variant of ReflectedStructs
template<template<typename> typename O,
         ::refl::ReflectedStruct... Ts>
struct TypeTraverse<O, ::base::variant<Ts...>> {
  using type = trv::list<typename O<Ts>::type...>;
};

// Rds variant
template<template<typename> typename O, typename S>
requires requires { typename S::i_am_rds_variant; }
struct TypeTraverse<O, S> {
  using type = trv::list<typename O<typename S::Base>::type>;
};

// WrapsReflected
template<template<typename> typename O, ::refl::WrapsReflected T>
struct TypeTraverse<O, T> {
  using type = trv::list<typename O<std::remove_cvref_t<
      decltype( std::declval<T>().refl() )>>::type>;
};

} // namespace trv

/****************************************************************
** Lua Traits
*****************************************************************/
namespace lua {

#define LUA_USERDATA_TRAITS_T( type, model ) \
  template<typename T>                       \
  struct type_traits<type<T>>                \
    : TraitsForModel<type<T>,                \
                     e_userdata_ownership_model::owned_by_cpp>

#define LUA_USERDATA_TRAITS_KV( type, model ) \
  template<typename K, typename V>            \
  struct type_traits<type<K, V>>              \
    : TraitsForModel<type<K, V>,              \
                     e_userdata_ownership_model::owned_by_cpp>

LUA_USERDATA_TRAITS_T( std::vector, owned_by_cpp ){};
LUA_USERDATA_TRAITS_T( std::deque, owned_by_cpp ){};
LUA_USERDATA_TRAITS_T( ::gfx::Matrix, owned_by_cpp ){};
LUA_USERDATA_TRAITS_KV( std::map, owned_by_cpp ){};
LUA_USERDATA_TRAITS_KV( std::unordered_map, owned_by_cpp ){};
LUA_USERDATA_TRAITS_KV( refl::enum_map, owned_by_cpp ){};

template<refl::ReflectedStruct S>
requires( !PushableViaAdl<S> && !GettableViaAdl<S> )
struct type_traits<S>
  : TraitsForModel<S, e_userdata_ownership_model::owned_by_cpp> {
};

template<refl::WrapsReflected S>
requires( !requires { typename type_traits<S>::type; } )
struct type_traits<S>
  : TraitsForModel<S, e_userdata_ownership_model::owned_by_cpp> {
};

template<refl::ReflectedStruct... Ts>
struct type_traits<base::variant<Ts...>>
  : TraitsForModel<base::variant<Ts...>,
                   e_userdata_ownership_model::owned_by_cpp> {};

template<typename S>
requires requires { typename S::i_am_rds_variant; }
struct type_traits<S>
  : TraitsForModel<S, e_userdata_ownership_model::owned_by_cpp> {
};

template<typename T, size_t N>
struct type_traits<std::array<T, N>>
  : TraitsForModel<std::array<T, N>,
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

void define_usertype_for( lua::state&, tag<std::string> ) {}

void define_usertype_for( lua::state&, tag<::rn::Coord> ) {}

void define_usertype_for( lua::state&,
                          tag<::rn::GenericUnitId> ) {}

void define_usertype_for( lua::state&,
                          tag<rn::MovementPoints> ) {}

template<typename T>
requires lua::Pushable<T> && lua::Gettable<T>
void define_usertype_for( lua::state&, tag<::base::maybe<T>> ) {
  // TBD: I think these would only be for lua types, like any.
}

template<typename T>
requires lua::Pushable<T> && lua::Gettable<T&>
[[maybe_unused]] void define_usertype_for(
    lua::state&, tag<::base::maybe<T>> ) {
  // TBD: for lua-owned C++ values.
}

template<typename T>
requires lua::Pushable<T&> && lua::Gettable<T&>
void define_usertype_for( lua::state& st,
                          tag<::base::maybe<T>> ) {
  using U      = ::base::maybe<T>;
  auto const L = st.resource();
  auto u       = st.usertype.create<U>();

  u["has_value"] = []( U& o ) { return o.has_value(); };
  u["reset"]     = []( U& o ) { o.reset(); };
  u["emplace"]   = []( U& o ) -> T& { return o.emplace(); };
  // This one will return either lua nil or userdata T&.
  u["value"] = [&]( U& o ) -> base::maybe<T&> { return o; };

  table mt           = u[metatable_key];
  auto const __index = mt["__index"].template as<rfunction>();

  mt["__index"] = [L, __index](
                      U& o, any const key ) -> base::maybe<any> {
    if( auto const member = __index( o, key ); member != nil )
      return member.template as<any>();
    if( !o.has_value() )
      lua::throw_lua_error(
          L, "cannot get field '{}' on an empty value.", key );
    any const underlying = as<any>( L, *o );
    return underlying[key];
  };

  mt["__newindex"] = [L]( U& o, any const key, any const val ) {
    if( !o.has_value() )
      lua::throw_lua_error(
          L, "cannot set field '{}' on an empty value.", key );
    any const underlying = as<any>( L, *o );
    underlying[key]      = val;
  };
}

template<template<typename K, typename V> typename M, typename K,
         typename V>
void define_usertype_for_map_impl( lua::state& st,
                                   tag<M<K, V>> ) {
  using U = M<K, V>;
  auto u  = st.usertype.create<U>();

  u["size"]  = []( U& o ) -> int { return o.size(); };
  u["clear"] = []( U& o ) { o.clear(); };

  table mt           = u[metatable_key];
  auto const __index = mt["__index"].template as<rfunction>();

  // Add some members here.

  mt["__index"] = [__index](
                      U& o, any const key ) -> base::maybe<any> {
    if( auto const member = __index( o, key ); member != nil )
      return member.template as<any>();
    auto const maybe_key = safe_as<K>( key );
    if( !maybe_key.has_value() ) return base::nothing;
    auto const iter = o.find( *maybe_key );
    if( iter == o.end() ) return base::nothing;
    auto const L = __index.this_cthread();
    return as<any>( L, iter->second );
  };

  using ValueType =
      std::conditional_t<lua::Gettable<V const&>, V const&, V>;

  mt["__newindex"] = []( U& o, K const& key,
                         ValueType const val ) { o[key] = val; };
}

template<typename K, typename V>
void define_usertype_for( lua::state& st, tag<std::map<K, V>> ) {
  define_usertype_for_map_impl( st, tag<std::map<K, V>>{} );
}

template<typename K, typename V>
void define_usertype_for( lua::state& st,
                          tag<std::unordered_map<K, V>> ) {
  define_usertype_for_map_impl(
      st, tag<std::unordered_map<K, V>>{} );
}

template<typename K, typename V>
void define_usertype_for( lua::state& st,
                          tag<::refl::enum_map<K, V>> ) {
  using U = ::refl::enum_map<K, V>;
  auto u  = st.usertype.create<U>();

  table mt           = u[metatable_key];
  auto const __index = mt["__index"].template as<rfunction>();

  // Add some members here.

  mt["__index"] = [__index](
                      U& o, any const key ) -> base::maybe<any> {
    if( auto const member = __index( o, key ); member != nil )
      return member.template as<any>();
    auto const maybe_key = safe_as<K>( key );
    if( !maybe_key.has_value() ) return base::nothing;
    auto& val    = o[*maybe_key];
    auto const L = __index.this_cthread();
    return as<any>( L, val );
  };

  using ValueType =
      std::conditional_t<lua::Gettable<V const&>, V const&, V>;

  mt["__newindex"] = []( U& o, K const& key,
                         ValueType const val ) { o[key] = val; };
}

template<refl::ReflectedStruct S>
void define_usertype_for( lua::state& st, tag<S> ) {
  auto u = st.usertype.create<S>();
  trv::traverse(
      refl::traits<S>::fields,
      [&]( auto& field, string_view const ) {
        using FieldType =
            std::remove_cvref_t<decltype( field )>::type;
        // Here the `name` passed into this lambda is e.g. "<0>",
        // "<1>", because we are traversing the tuple. So we need
        // field.name to get the real field name.
        if constexpr( !std::is_same_v<FieldType,
                                      ::rn::UnitComposition> )
          u[field.name] = field.accessor;
      } );
}

template<refl::WrapsReflected S>
void define_usertype_for( lua::state&, tag<S> ) {
  // Default specialization... probably will need to customize
  // this for each wrapped type.
}

template<typename U, refl::ReflectedStruct... Ts>
void define_usertype_rds_variant_impl(
    base::variant<Ts...> const*, auto& u ) {
  using V                   = base::variant<Ts...>;
  static auto const& kNames = refl::alternative_names<V>();

  table mt           = u[metatable_key];
  auto const __index = mt["__index"].template as<rfunction>();

  // Add some members here.

  mt["__index"] = [__index](
                      U& o, any const key ) -> base::maybe<any> {
    base::maybe<any> res;
    if( auto const member = __index( o, key ); member != nil ) {
      res = member.template as<any>();
      return res;
    }
    auto const maybe_key = safe_as<string>( key );
    if( !maybe_key.has_value() ) return res;
    auto const L = __index.this_cthread();
    [&]<size_t... I>( index_sequence<I...> ) {
      auto const fn =
          [&]<size_t Idx>( integral_constant<size_t, Idx> ) {
            static_assert( Idx < variant_size_v<V> );
            if( o.index() == Idx ) {
              if( *maybe_key == kNames[Idx] )
                res = as<any>( L, std::get<Idx>( o ) );
            }
          };
      ( fn( integral_constant<size_t, I>{} ), ... );
    }( make_index_sequence<sizeof...( Ts )>() );
    return res;
  };

  mt["__newindex"] = []( U& o, string const& key, any const ) {
    [&]<size_t... I>( index_sequence<I...> ) {
      auto const fn =
          [&]<size_t Idx>( integral_constant<size_t, Idx> ) {
            static_assert( Idx < variant_size_v<V> );
            if( kNames[Idx] == key ) o.template emplace<Idx>();
          };
      ( fn( integral_constant<size_t, I>{} ), ... );
    }( make_index_sequence<sizeof...( Ts )>() );
  };
}

template<refl::ReflectedStruct... Ts>
void define_usertype_for( lua::state& st,
                          tag<base::variant<Ts...>> ) {
  using U = base::variant<Ts...>;
  auto u  = st.usertype.create<U>();
  define_usertype_rds_variant_impl<U>( (U*){}, u );
}

template<typename S>
requires requires { typename S::i_am_rds_variant; }
void define_usertype_for( lua::state& st, tag<S> ) {
  auto u = st.usertype.create<S>();
  define_usertype_rds_variant_impl<S>( (typename S::Base*){},
                                       u );
}

template<typename T>
void define_usertype_for( lua::state& st, tag<std::deque<T>> ) {
  using U = std::deque<T>;
  auto u  = st.usertype.create<U>();

  u["size"] = []( U& o ) -> int { return o.size(); };
}

template<typename T>
void define_usertype_for( lua::state& st, tag<std::vector<T>> ) {
  using U = std::vector<T>;
  auto u  = st.usertype.create<U>();

  u["size"] = []( U& o ) -> int { return o.size(); };
}

template<typename T>
void define_usertype_for( lua::state& st, tag<gfx::Matrix<T>> ) {
  using U = gfx::Matrix<T>;
  auto u  = st.usertype.create<U>();

  u["size"] = [&]( U& o ) -> lua::table {
    lua::table tbl     = st.table.create();
    gfx::size const sz = o.size();
    tbl["w"]           = sz.w;
    tbl["h"]           = sz.h;
    return tbl;
  };
}

template<typename T, size_t N>
void define_usertype_for( lua::state& st,
                          tag<std::array<T, N>> ) {
  using U = std::array<T, N>;
  auto u  = st.usertype.create<U>();

  u["size"] = []( U& ) -> int { return N; };

  table mt           = u[metatable_key];
  auto const __index = mt["__index"].template as<rfunction>();

  mt["__index"] = [&, L = st.resource()](
                      U& o, any const key ) -> any {
    if( auto const member = __index( o, key ); member != nil ) {
      return member.template as<any>();
    }
    size_t const idx = key.as<int64_t>();
    LUA_CHECK( st, idx >= 1 && idx <= N,
               "array index out of bounds" );
    return as<any>( L, o[idx - 1] );
  };

  mt["__newindex"] = [&]( U& obj, int idx, T type ) {
    LUA_CHECK( st, idx >= 1 && idx <= 3,
               "immigrant pool index must be either 1, 2, 3." );
    obj[idx - 1] = type;
  };
}

} // namespace
} // namespace lua

/****************************************************************
** RegisterLuaType
*****************************************************************/
namespace rn {

template<typename T>
struct RegisterLuaType {
  using type = ::trv::TypeTraverse<RegisterLuaType, T>::type;

  // Ensure that we're never Gettable in two different ways.
  static_assert( lua::Gettable<T&> != lua::Gettable<T> );

#if 0
  // Ensure that we're never pushable in two different ways.
  inline static auto const static_asserts = [] {
    if constexpr( !std::is_same_v<T, ::rn::UnitComposition> &&
                  !std::is_scalar_v<T> )
      static_assert( lua::Pushable<T&> != lua::Pushable<T> );
  };
#endif

  inline static int _ = [] {
    static auto constexpr register_fn = +[]( lua::state& st ) {
      ::lua::define_usertype_for( st, ::lua::tag<T>{} );
    };
    static auto constexpr p_register_fn = +register_fn;
    ::lua::detail::register_lua_fn( &p_register_fn );
    return 0;
  }();
  ODR_USE_MEMBER( _ );
};

TRV_RUN_TYPE_TRAVERSE( RegisterLuaType, RootState );

} // namespace rn

/****************************************************************
** Linker.
*****************************************************************/
namespace rn {
void linker_dont_discard_module_ss_lua_root();
void linker_dont_discard_module_ss_lua_root() {}
} // namespace rn
