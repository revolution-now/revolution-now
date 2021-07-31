/****************************************************************
**combinator.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Parser combinators.
*
*****************************************************************/
#pragma once

// parz
#include "parser.hpp"
#include "promise.hpp"

// C++ standard library
#include <type_traits>
#include <vector>

namespace parz {

/****************************************************************
** repeated
*****************************************************************/
// Parses zero or more of the given parser.
struct Repeated {
  // This is a struct instead of a function to work around a
  // clang issue where it doesn't like coroutine function tem-
  // plates.
  template<typename Func>
  auto operator()( Func&& f ) const -> parser<std::vector<
      typename std::invoke_result_t<Func>::value_type>> {
    using res_t =
        typename std::invoke_result_t<Func>::value_type;
    std::vector<res_t> res;
    while( true ) {
      auto m = co_await Try{ f() };
      if( !m ) break;
      res.push_back( std::move( *m ) );
    }
    co_return res;
  }
};

inline constexpr Repeated repeated{};

/****************************************************************
** some
*****************************************************************/
// Parses one or more of the given parser.
struct Some {
  // This is a struct instead of a function to work around a
  // clang issue where it doesn't like coroutine function tem-
  // plates.
  template<typename Func>
  auto operator()( Func&& f ) const -> parser<std::vector<
      typename std::invoke_result_t<Func>::value_type>> {
    using res_t =
        typename std::invoke_result_t<Func>::value_type;
    std::vector<res_t> res = co_await repeated( f );
    if( res.empty() ) co_await parz::error( "" );
    co_return res;
  }
};

inline constexpr Some some{};

/****************************************************************
** Spaces
*****************************************************************/
// Consumes one space char or fails.
parser<> space_char();

// Consumes zero or more spaces.
parser<> eat_spaces();

/****************************************************************
** Character Classes
*****************************************************************/
// Consumes a char that must be c, otherwise it fails.
parser<> exact_char( char c );

// Consumes one identifier char or fails.
parser<char> identifier_char();

// Consumes one digit [0-9] char or fails.
parser<char> digit();

} // namespace parz
