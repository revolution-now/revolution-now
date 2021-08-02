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

// base
#include "base/fmt.hpp"
#include "base/maybe.hpp"

// C++ standard library
#include <string_view>

/****************************************************************
** "Magic" Parser Awaitables
*****************************************************************/
// The types in this section are recognized specially by the
// promise type when awaited on, and have special access to the
// insides of the promise. This is for the purpose of providing
// some basic combinators as a starting point as well as im-
// proving performance (a builtin version of a parser will be
// much faster than one implemented in terms of primitives).
namespace parz {

// When a parser is wrapped in this object it will return a may-
// be<T> instead of a T and it will thus be allowed to fail
// without failing the entire parsing operation.
template<Parser P>
struct Try {
  using value_type = base::maybe<typename P::value_type>;
  Try( P&& p_ ) : p( std::move( p_ ) ) {}
  P p;
};

// This will immediately fail the parser.
struct fail {
  using value_type = std::monostate;
  fail()           = default;
  fail( std::string_view msg ) : err( msg ) {}
  fail( error&& e ) : err( std::move( e ) ) {}
  error err;
};

/****************************************************************
** Builtin Parsers
*****************************************************************/
// Note that these all return string_views into the buffer.
struct BuiltinParseResult {
  std::string_view sv;
  // This may be different from the length of sv, e.g. if we are
  // parsing a quoted string "hello" and we want to return only
  // hello.
  int consumed;
};

// This gets the next character from the buffer and fails if
// there are no more characters.
struct builtin_next_char {
  using value_type = char;
};

// Removes blanks.
struct builtin_blanks {
  using value_type = std::string_view;
  base::maybe<BuiltinParseResult> try_parse(
      std::string_view in ) const;
};

struct builtin_identifier {
  using value_type = std::string_view;
  base::maybe<BuiltinParseResult> try_parse(
      std::string_view in ) const;
};

struct builtin_single_quoted {
  using value_type = std::string_view;
  base::maybe<BuiltinParseResult> try_parse(
      std::string_view in ) const;
};

struct builtin_double_quoted {
  using value_type = std::string_view;
  base::maybe<BuiltinParseResult> try_parse(
      std::string_view in ) const;
};

} // namespace parz

DEFINE_FORMAT( parz::BuiltinParseResult,
               "BuiltinParseResult{{sv={}, consumed={}}}", o.sv,
               o.consumed );
