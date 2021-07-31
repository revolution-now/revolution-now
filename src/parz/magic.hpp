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

// parz
#include "parser.hpp"

/****************************************************************
** "Magic" Parser Awaitables
*****************************************************************/
// The types in this section are recognized specially by the
// promise type when awaited on, and have special access to the
// insides of the promise. They allow accessing the buffer (to
// get the next character) and backtracking a parser. From these
// two primitives it should be possible to construct all other
// parsers.
namespace parz {

// This gets the next character from the buffer and fails if
// there are no more characters.
struct next_char {};

// When a parser is wrapped in this object it will return a may-
// be<T> instead of a T and it will thus be allowed to fail
// without failing the entire parsing operation.
template<typename T>
struct Try {
  parser<T> p;
};

template<typename P>
Try( P ) -> Try<typename P::value_type>;

} // namespace parz
