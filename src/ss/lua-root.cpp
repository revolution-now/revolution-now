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
#include "luapp/ext-refl.hpp"
#include "luapp/ext-std.hpp"
#include "luapp/ext-userdata.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"
#include "luapp/usertype.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/ext-type-traverse.hpp"
#include "refl/ext.hpp"
#include "refl/query-struct.hpp"
#include "refl/to-str.hpp"

// traverse
#include "traverse/ext-std.hpp"
#include "traverse/ext.hpp"
#include "traverse/type-ext-base.hpp"
#include "traverse/type-ext-std.hpp"
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
** Lua Usertypes
*****************************************************************/
namespace lua {
namespace {

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
requires( !lua::PushableViaAdl<S> && !lua::GettableViaAdl<S> )
void define_usertype_for( lua::state& st, tag<S> ) {
  auto u = st.usertype.create<S>();
  trv::traverse(
      refl::traits<S>::fields,
      // Here the `name` passed into this lambda is e.g. "<0>",
      // "<1>", because we are traversing the tuple. So we need
      // field.name to get the real field name.
      [&]( auto& field, string_view const /*name*/ ) {
        using FieldType =
            std::remove_cvref_t<decltype( field )>::type;
        if constexpr( !HasValueUserdataOwnershipModel<
                          FieldType> )
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
namespace lua {

template<typename T>
concept HasUsertypeDefinition = requires( state& st, tag<T> ) {
  { define_usertype_for( st, tag<T>{} ) } -> std::same_as<void>;
};

template<typename T>
concept HasUniqueOwnershipCategory =
    Gettable<T> + Gettable<T&> + Gettable<T&&> == 1;

template<typename T>
struct RegisterLuaType {
  using type = ::trv::TypeTraverse<RegisterLuaType, T>::type;

  // Ensure that we're never Gettable in more than one way.
  static_assert( HasUniqueOwnershipCategory<T> );
  static_assert( !Gettable<T const&&> );

  inline static int _ = [] {
    if constexpr( HasUsertypeDefinition<T> ) {
      static auto constexpr register_fn = +[]( state& st ) {
        define_usertype_for( st, tag<T>{} );
      };
      static auto constexpr p_register_fn = +register_fn;
      detail::register_lua_fn( &p_register_fn );
    }
    return 0;
  }();
  ODR_USE_MEMBER( _ );
};

TRV_RUN_TYPE_TRAVERSE( RegisterLuaType, ::rn::RootState );

} // namespace lua

/****************************************************************
** Linker.
*****************************************************************/
namespace rn {
void linker_dont_discard_module_ss_lua_root();
void linker_dont_discard_module_ss_lua_root() {}
} // namespace rn
