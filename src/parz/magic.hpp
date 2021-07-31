/****************************************************************
**magic.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: "Magic" parser waitables that have special powers.
*
*****************************************************************/
#pragma once

#include "concepts.hpp"
#include "error.hpp"

// C++ standard library
#include <string_view>

/****************************************************************
** "Magic" Parser Awaitables
*****************************************************************/
// The types in this section are recognized specially by the
// promise type when awaited on, and have special access to the
// insides of the promise. They allow accessing the buffer (to
// get the next character) and backtracking a parser. From these
// three primitives it should be possible to construct all other
// parsers.
namespace parz {

// This gets the next character from the buffer and fails if
// there are no more characters.
struct next_char {};

// When a parser is wrapped in this object it will return a may-
// be<T> instead of a T and it will thus be allowed to fail
// without failing the entire parsing operation.
template<Parser P>
struct Try {
  Try( P&& p_ ) : p( std::move( p_ ) ) {}
  P p;
};

// This will immediately fail the parser.
struct fail {
  fail() = default;
  fail( std::string_view msg ) : err( msg ) {}
  fail( error&& e ) : err( std::move( e ) ) {}
  error err;
};

} // namespace parz
