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

// Revolution Now
#include "ext.hpp"

// C++ standard library
#include <tuple>

namespace lua {

/****************************************************************
** std::monostate
*****************************************************************/
// std::monostate just gets pushed as nil, and will only success-
// fully pop if there is a value of nil. The reason for this is
// that std::monostate is isomorphic to nil, in the sense that it
// is a type with just a single value.
void lua_push( cthread L, std::monostate const& );

base::maybe<std::monostate> lua_get( cthread L, int idx,
                                     tag<std::monostate> );

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
  static base::maybe<Tuple> get( cthread L, int idx, tag<Tuple> )
    requires ( Gettable<Ts> && ... ) {
    // clang-format on
    return get_impl(
        L, idx, std::make_index_sequence<sizeof...( Ts )>() );
  }

  // clang-format off
private:
  // clang-format on

  template<size_t... Idx>
  static void
  push_impl( cthread L, std::tuple<Ts...> const& tup,
             std::index_sequence<Idx...> ) {
    ( lua::push( L, std::get<Idx>( tup ) ), ... );
  }

  template<size_t... Idx>
  static base::maybe<Tuple> get_impl(
      cthread L, int idx, std::index_sequence<Idx...> ) {
    auto tuple_of_maybes =
        std::tuple{ lua::get<std::tuple_element_t<Idx, Tuple>>(
            L, idx - sizeof...( Idx ) + 1 + Idx )... };

    bool failed = false;
    auto check_maybes =
        [&]<size_t I>( std::integral_constant<size_t, I> ) {
          auto& m = std::get<I>( tuple_of_maybes );
          if( !m.has_value() ) failed = true;
        };
    ( check_maybes( std::integral_constant<size_t, Idx>{} ),
      ... );
    if( failed ) return base::nothing;

    auto get_maybe =
        [&]<size_t I>( std::integral_constant<size_t, I> )
        -> decltype( auto ) {
      return *std::get<I>( tuple_of_maybes );
    };
    return std::tuple<Ts...>{
        get_maybe( std::integral_constant<size_t, Idx>{} )... };
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
  static base::maybe<Pair> get( cthread L, int idx, tag<Pair> )
    requires ( Gettable<A> && Gettable<B> ) {
    // clang-format on
    base::maybe<A> ma = lua::get<A>( L, idx - 1 );
    base::maybe<B> mb = lua::get<B>( L, idx );
    if( !ma or !mb ) return base::nothing;
    return Pair{ *ma, *mb };
  }

  // clang-format off
private:
  // clang-format on
};

} // namespace lua
