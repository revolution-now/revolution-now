/****************************************************************
**lambda.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-08.
*
* Description: Some useful lambda function as macros.
*
*****************************************************************/
#pragma once

// C++ standard library.
#include <utility>

#define LIFT( f )                                      \
  []<typename... Args>( Args && ... args ) noexcept(   \
      noexcept( f( std::forward<Args>( args )... ) ) ) \
      ->decltype( auto ) {                             \
    return f( std::forward<Args>( args )... );         \
  }

// This is intended to lessen typing for the simplest of lambda
// functions, namely, those which have no captures, take one or
// two const ref parameters, and consist of either a single re-
// turn statement or just a single expression.
#define L( a ) []( auto const& _ ) { return a; }
#define L_( a ) []( auto const& _ ) { a; }

#define L0( a ) [] { return a; }
#define L0_( a ) [] { a; }

#define L2( a ) \
  []( auto const& _1, auto const& _2 ) { return a; }
#define L2_( a ) []( auto const& _1, auto const& _2 ) { a; }

// One  for  lambdas  that  capture  all (usually for
// simplicity).
#define LC( a ) [&]( auto const& _ ) { return a; }
#define LC_( a ) [&]( auto const& _ ) { a; }

#define LC2( a ) \
  [&]( auto const& _1, auto const& _2 ) { return a; }
#define LC2_( a ) [&]( auto const& _1, auto const& _2 ) { a; }

#define LC0( a ) [&] { return a; }
#define LC0_( a ) [&] { a; }

namespace base::detail {

struct not_a_parameter {};

template<int N, typename T, typename... Ts>
constexpr decltype( auto ) nth_impl( T&& t, Ts&&... ts ) {
  if constexpr( N == 0 ) {
    return std::forward<T>( t );
  } else {
    return nth_impl<N - 1>( std::forward<Ts>( ts )... );
  }
}

template<int N, typename... Ts>
constexpr decltype( auto ) nth( Ts&&... ts ) {
  if constexpr( N < sizeof...( Ts ) ) {
    return nth_impl<N>( std::forward<Ts>( ts )... );
  } else {
    return not_a_parameter{};
  }
}

} // namespace base::detail

// This one you can use like this:
//
//   auto f = [&] λ( _1 + _2 );
//
//   auto g = [=] λ( _ * 2 );
//
#define λ( ... )                                               \
  <typename... T>( T && ... _args ) {                          \
    [[maybe_unused]] auto&& _ =                                \
        ::base::detail::nth<0>( std::forward<T>( _args )... ); \
    [[maybe_unused]] auto&& _1 =                               \
        ::base::detail::nth<0>( std::forward<T>( _args )... ); \
    [[maybe_unused]] auto&& _2 =                               \
        ::base::detail::nth<1>( std::forward<T>( _args )... ); \
    [[maybe_unused]] auto&& _3 =                               \
        ::base::detail::nth<2>( std::forward<T>( _args )... ); \
    [[maybe_unused]] auto&& _4 =                               \
        ::base::detail::nth<3>( std::forward<T>( _args )... ); \
    return __VA_ARGS__;                                        \
  }

// prevent weird gcc warning ("backslash-newline at end of file")
#define XYZXYZXYZ