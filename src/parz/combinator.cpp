/****************************************************************
**combinator.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Parser combinators.
*
*****************************************************************/
#include "combinator.hpp"

// parz
#include "parser.hpp"
#include "promise.hpp"

using namespace std;

namespace parz {

parser<> eat_spaces() { co_await repeated( space_char ); }

parser<> exact_char( char c ) {
  char next = co_await next_char{};
  if( next != c ) co_await error( "" );
}

parser<> space_char() {
  char next = co_await next_char{};
  if( next != ' ' && next != '\n' ) co_await parz::error( "" );
}

parser<char> identifier_char() {
  char c = co_await next_char{};
  if( !( ( c >= 'a' && c <= 'z' ) || c == '_' ) )
    co_await parz::error( "" );
  co_return c;
}

parser<char> digit() {
  char c = co_await next_char{};
  if( !( c >= '0' && c <= '9' ) ) co_await parz::error( "" );
  co_return c;
}

} // namespace parz
