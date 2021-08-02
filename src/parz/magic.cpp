/****************************************************************
**magic.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-02.
*
* Description: "Magic" parsers that have special powers.
*
*****************************************************************/
#include "magic.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace parz {

namespace {

using base::maybe;
using base::nothing;

bool is_blank( char c ) {
  return ( c == ' ' ) || ( c == '\n' ) || ( c == '\r' ) ||
         ( c == '\t' );
}

bool is_leading_identifier_char( char c ) {
  return ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ) ||
         ( c == '_' );
}

bool is_identifier_char( char c ) {
  return ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'Z' ) ||
         ( c >= 'a' && c <= 'z' ) || ( c == '_' );
}

} // namespace

// Removes blanks.
maybe<BuiltinParseResult> builtin_blanks::try_parse(
    string_view in ) const {
  int pos = 0;
  while( pos < int( in.size() ) && is_blank( in[pos] ) ) ++pos;
  auto res = BuiltinParseResult{ .sv       = in.substr( 0, pos ),
                                 .consumed = pos };
  return res;
};

maybe<BuiltinParseResult> builtin_identifier::try_parse(
    string_view in ) const {
  if( in.empty() || !is_leading_identifier_char( in[0] ) )
    return nothing;
  int pos = 0;
  while( pos < int( in.size() ) &&
         is_identifier_char( in[pos] ) )
    ++pos;
  return BuiltinParseResult{ .sv       = in.substr( 0, pos ),
                             .consumed = pos };
};

maybe<BuiltinParseResult> builtin_single_quoted::try_parse(
    string_view in ) const {
  int pos = 0;
  if( in.empty() || in[pos++] != '\'' ) return nothing;
  while( true ) {
    // EOF before closing quote?
    if( pos == int( in.size() ) ) return nothing;
    if( in[pos++] == '\'' ) break;
  }
  DCHECK( pos >= 2 );
  string_view res( in.begin() + 1, pos - 2 );
  return BuiltinParseResult{ .sv = res, .consumed = pos };
};

maybe<BuiltinParseResult> builtin_double_quoted::try_parse(
    string_view in ) const {
  int pos = 0;
  if( in.empty() || in[pos++] != '"' ) return nothing;
  while( true ) {
    // EOF before closing quote?
    if( pos == int( in.size() ) ) return nothing;
    if( in[pos++] == '"' ) break;
  }
  DCHECK( pos >= 2 );
  string_view res( in.begin() + 1, pos - 2 );
  return BuiltinParseResult{ .sv = res, .consumed = pos };
};

} // namespace parz
