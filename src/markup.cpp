/****************************************************************
**markup.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-09.
*
* Description: Parser for text markup mini-language.
*
*****************************************************************/
#include "markup.hpp"

// base
#include "base/string.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::expect;
using ::base::unexpected;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
expect<MarkedUpText> parse_markup( string_view text ) {
  vector<string> lines = base::str_split( text, '\n' );
  MarkedUpText   res;
  auto&          line_frags = res.chunks;
  line_frags.reserve( lines.size() );
  MarkupStyle curr_style{};
  for( string_view const line : lines ) {
    vector<MarkedUpChunk> line_mkup;
    auto                  start = line.begin();
    auto                  end   = line.end();
    while( true ) {
      auto left_bracket = find( start, end, '[' );
      if( left_bracket - start > 0 )
        line_mkup.push_back(
            { string( start, left_bracket - start ),
              curr_style } );
      if( left_bracket == end ) break;
      CHECK( *left_bracket == '[', "internal parser error" );
      if( end - left_bracket < 2 )
        return unexpected<MarkedUpText>(
            "markup syntax error: `[` must have a corresponding "
            "`]`" );
      curr_style         = MarkupStyle{ .highlight = true };
      start              = left_bracket + 1;
      auto right_bracket = find( start, end, ']' );
      if( right_bracket == end )
        return unexpected<MarkedUpText>(
            "markup syntax error: `[` must have a corresponding "
            "`]`" );
      if( right_bracket - start > 0 )
        line_mkup.push_back(
            { string( start, right_bracket - start ),
              curr_style } );
      curr_style = MarkupStyle{};
      start      = right_bracket + 1;
    }
    if( !line_mkup.empty() )
      line_frags.push_back( std::move( line_mkup ) );
  }
  return res;
}

string remove_markup( string_view text ) {
  string res;
  // The result will always be the same or smaller.
  res.reserve( text.size() );
  int pos = 0;
  while( pos < int( text.size() ) ) {
    if( char c = text[pos++]; c != '[' ) {
      res.push_back( c );
      continue;
    }
    while( pos < int( text.size() ) && text[pos] != ']' )
      res.push_back( text[pos++] );
    if( pos < int( text.size() ) && text[pos] == ']' ) ++pos;
  }
  return res;
}

} // namespace rn
