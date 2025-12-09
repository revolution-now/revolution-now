/****************************************************************
**ext-std.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-26.
*
* Description: Lua push/get extensions for std library types.
*
*****************************************************************/
#pragma once

// luapp
#include "ext-base.hpp" // For lambdas returning maybe<T> to lua.
#include "ext-userdata.hpp"
#include "ext-usertype.hpp"
#include "ext.hpp"
#include "state.hpp"

// base
#include "base/to-str-ext-std.hpp"
#include "base/to-str.hpp"

// C++ standard library
#include <array>
#include <deque>
#include <map>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace lua {

/****************************************************************
** std::tuple
*****************************************************************/
template<typename... Ts>
struct type_traits<std::tuple<Ts...>> {
  using Tuple = std::tuple<Ts...>;

  static constexpr int nvalues = ( nvalues_for<Ts>() + ... );
  static_assert(
      nvalues == sizeof...( Ts ),
      "this push/get implementation for tuple will not work if "
      "any of the tuple element types have nvalues != 1" );

  // clang-format off
  static void push( cthread L, Tuple const& tup )
    requires ( Pushable<Ts> && ... ) {
    // clang-format on
    push_impl( L, tup,
               std::make_index_sequence<sizeof...( Ts )>() );
  }

  // clang-format off
  static lua_expect<Tuple> get( cthread L, int idx, tag<Tuple> )
    requires ( Gettable<Ts> && ... ) {
    // clang-format on
    return get_impl(
        L, idx, std::make_index_sequence<sizeof...( Ts )>() );
  }

  // clang-format off
private:
  // clang-format on

  template<size_t... Idx>
  static void push_impl( cthread L, std::tuple<Ts...> const& tup,
                         std::index_sequence<Idx...> ) {
    ( lua::push( L, std::get<Idx>( tup ) ), ... );
  }

  template<size_t... Idx>
  static lua_expect<Tuple> get_impl(
      cthread L, int idx, std::index_sequence<Idx...> ) {
    auto tuple_of_expects =
        std::tuple{ lua::get<std::tuple_element_t<Idx, Tuple>>(
            L, idx - sizeof...( Idx ) + 1 + Idx )... };

    bool failed = false;
    auto check_expects =
        [&]<size_t I>( std::integral_constant<size_t, I> ) {
          auto& m = std::get<I>( tuple_of_expects );
          if( !m.has_value() ) failed = true;
        };
    ( check_expects( std::integral_constant<size_t, Idx>{} ),
      ... );
    if( failed ) return unexpected{};

    auto get_expect =
        [&]<size_t I>( std::integral_constant<size_t, I> )
        -> decltype( auto ) {
      return *std::get<I>( tuple_of_expects );
    };
    return std::tuple<Ts...>{
      get_expect( std::integral_constant<size_t, Idx>{} )... };
  }
};

/****************************************************************
** std::pair
*****************************************************************/
template<typename A, typename B>
struct type_traits<std::pair<A, B>> {
  using Pair = std::pair<A, B>;

  static constexpr int nvalues =
      nvalues_for<A>() + nvalues_for<B>();
  static_assert(
      nvalues == 2,
      "this push/get implementation for pair will not work if "
      "any of the pair element types have nvalues != 1" );

  // clang-format off
  static void push( cthread L, Pair const& p )
    requires ( Pushable<A> && Pushable<B> ) {
    // clang-format on
    lua::push( L, p.first );
    lua::push( L, p.second );
  }

  // clang-format off
  static lua_expect<Pair> get( cthread L, int idx, tag<Pair> )
    requires ( Gettable<A> && Gettable<B> ) {
    // clang-format on
    lua_expect<A> ma = lua::get<A>( L, idx - 1 );
    lua_expect<B> mb = lua::get<B>( L, idx );
    if( !ma or !mb ) return unexpected{};
    return Pair{ *ma, *mb };
  }

  // clang-format off
private:
  // clang-format on
};

/****************************************************************
** Maps (Generic)
*****************************************************************/
// Lua API for maps:
//
//   local m = ...map_type<string, int>...
//
//   assert( m:size() == 0 )
//
//   m["hello"] = 7
//   m.hello = 8
//   print( m['hello'] )
//
//   -- If the value type is a userdata T then:
//   local obj = m:make( 'world' )
//   obj.xyz = ...
//   obj['world'].xyz = ...
//
template<template<typename K, typename V> typename M, typename K,
         typename V>
void define_usertype_for_map_impl( state& st, tag<M<K, V>> ) {
  using U = M<K, V>;
  auto u  = st.usertype.create<U>();

  table mt           = u[metatable_key];
  auto const __index = mt["__index"].template as<rfunction>();
  cthread const L    = st.resource();

  u["size"]  = []( U& o ) -> int { return o.size(); };
  u["clear"] = []( U& o ) { o.clear(); };
  u["make"]  = []( U& o, K const& key ) -> V& {
    return o.emplace( key, V{} ).first->second;
  };

  mt["__index"] = [L, __index](
                      U& o, any const key ) -> base::maybe<any> {
    if( auto const member = __index( o, key ); member != nil )
      return member.template as<any>();
    auto const maybe_key = safe_as<K>( key );
    if( !maybe_key.has_value() ) return base::nothing;
    auto const iter = o.find( *maybe_key );
    if( iter == o.end() ) return base::nothing;
    return as<any>( L, iter->second );
  };

  mt["__newindex"] = []( U& o, K const& key, V const& val ) {
    o[key] = val;
  };
}

/****************************************************************
** std::map
*****************************************************************/
LUA_USERDATA_TRAITS_KV( std::map, owned_by_cpp ){};

// Lua API for std::map:
//
//   ==> see the Maps (Generic) section.
//
template<typename K, typename V>
void define_usertype_for( state& st, tag<std::map<K, V>> ) {
  using M = std::map<K, V>;
  define_usertype_for_map_impl( st, tag<M>{} );
}

/****************************************************************
** std::unordered_map
*****************************************************************/
LUA_USERDATA_TRAITS_KV( std::unordered_map, owned_by_cpp ){};

// Lua API for std::unordered_map:
//
//   ==> see the Maps (Generic) section.
//
template<typename K, typename V>
void define_usertype_for( state& st,
                          tag<std::unordered_map<K, V>> ) {
  using M = std::unordered_map<K, V>;
  define_usertype_for_map_impl( st, tag<M>{} );
}

/****************************************************************
** std::vector
*****************************************************************/
LUA_USERDATA_TRAITS_T( std::vector, owned_by_cpp ){};

// Lua API for std::vector:
//
//   local v = ...vector of int...
//
//   assert( v:size() == 0 )
//
//   v:add()
//   v:add()
//   assert( v:size() == 2
//
//   v[1] = 4
//   v[2] = 5
//
//   -- Throws:
//   -- v[3] = 6
//
//   assert( v:size() == 2
//
//   print( v[1] )
//   print( v[2] )
//
//   v:clear()
//   assert( v:size() == 0 )
//
template<typename T>
void define_usertype_for( lua::state& st, tag<std::vector<T>> ) {
  using U = std::vector<T>;
  auto u  = st.usertype.create<U>();

  table mt           = u[metatable_key];
  auto const __index = mt["__index"].template as<rfunction>();
  cthread const L    = st.resource();

  u["size"]  = []( U& o ) -> int { return o.size(); };
  u["clear"] = []( U& o ) { o.clear(); };
  u["add"]   = []( U& o ) -> T& { return o.emplace_back(); };

  // NOTE: Indices start at 1. Also, in the below we don't return
  // nil on an out-of-bounds index here as a Lua array would be-
  // cause we don't want to give the player the impression that
  // this behaves like a Lua array: you have to add to the end
  // using the above `add` method before setting an element. If
  // the new element added is an object then you can use the re-
  // turn value (reference) to set members. Otherwise you have to
  // set using the __newindex approach, e.g. v[2] = 1.

  mt["__index"] = [L, __index](
                      U& o, any const key ) -> base::maybe<any> {
    auto const st = lua::state::view( L );
    if( auto const member = __index( o, key ); member != nil )
      return member.template as<any>();
    auto const expect_key = safe_as<int>( key );
    LUA_CHECK( st, expect_key.has_value(),
               "expected integer for index" );
    size_t const idx = *expect_key;
    LUA_CHECK( st, idx >= 1, "index must be >= 1" );
    LUA_CHECK( st, idx <= o.size(),
               "index out of bounds: {} > {}", idx, o.size() );
    CHECK_LT( idx - 1, o.size() );
    auto const L = __index.this_cthread();
    return as<any>( L, o[idx - 1] );
  };

  mt["__newindex"] = [L]( U& o, size_t const idx,
                          T const& val ) {
    auto const st = lua::state::view( L );
    LUA_CHECK( st, idx >= 1, "index must be >= 1" );
    LUA_CHECK( st, idx <= o.size(),
               "index out of bounds: {} > {}", idx, o.size() );
    o[idx - 1] = val;
  };
}

/****************************************************************
** std::array
*****************************************************************/
template<typename T, size_t N>
struct type_traits<std::array<T, N>>
  : TraitsForModel<std::array<T, N>,
                   e_userdata_ownership_model::owned_by_cpp> {};

// Lua API for std::array:
//
//   local a = ...std::array<int, 3>...
//
//   assert( a:size() == 3 ) -- will never change.
//
//   a[1] = 4
//   a[2] = 5
//   a[3] = 6
//
//   -- Throws:
//   -- a[4] = 7
//
//   assert( a:size() == 3
//
//   print( a[1] )
//   print( a[2] )
//
template<typename T, size_t N>
void define_usertype_for( lua::state& st,
                          tag<std::array<T, N>> ) {
  using U = std::array<T, N>;
  auto u  = st.usertype.create<U>();

  u["size"] = []( U& ) -> int { return N; };

  table mt           = u[metatable_key];
  auto const __index = mt["__index"].template as<rfunction>();
  cthread const L    = st.resource();

  mt["__index"] = [L, __index]( U& o, any const key ) -> any {
    auto const st = lua::state::view( L );
    if( auto const member = __index( o, key ); member != nil )
      return member.template as<any>();
    size_t const idx = key.as<int64_t>();
    LUA_CHECK( st, idx >= 1 && idx <= N,
               "array index out of bounds" );
    return as<any>( L, o[idx - 1] );
  };

  mt["__newindex"] = [L]( U& o, size_t const idx,
                          T const& val ) {
    auto const st = lua::state::view( L );
    LUA_CHECK( st, idx >= 1, "index must be >= 1" );
    LUA_CHECK( st, idx <= o.size(),
               "index out of bounds: {} > {}", idx, o.size() );
    o[idx - 1] = val;
  };
}

/****************************************************************
** std::deque
*****************************************************************/
LUA_USERDATA_TRAITS_T( std::deque, owned_by_cpp ){};

template<typename T>
void define_usertype_for( lua::state& st, tag<std::deque<T>> ) {
  using U = std::deque<T>;
  auto u  = st.usertype.create<U>();

  u["size"] = []( U& o ) -> int { return o.size(); };

  // TBD: probably won't need anything here.
}

} // namespace lua
