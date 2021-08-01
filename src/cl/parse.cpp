/****************************************************************
**parse.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Parser for config files.
*
*****************************************************************/
#include "parse.hpp"

// parz
#include "cl/ext-parse.hpp"
#include "parz/runner.hpp"

// base
#include "base/error.hpp"
#include "base/io.hpp"

using namespace std;

namespace cl {

base::valid_or<int> blankify_nested_comments(
    std::string& text ) {
  if( text.empty() ) return base::valid;
  int level = 0;
  int i     = 0;
  for( ; i < int( text.size() - 1 ); ++i ) {
    char c  = text[i];
    char c2 = text[i + 1];
    if( c == '\n' || c == '\r' ) continue;
    enum class type { normal, starting, ending };
    type t = ( c == '/' && c2 == '*' )   ? type::starting
             : ( c == '*' && c2 == '/' ) ? type::ending
                                         : type::normal;
    if( level == 0 ) {
      switch( t ) {
        case type::normal: //
          continue;
        case type::starting: //
          ++level;
          text[i] = ' ';
          ++i;
          text[i] = ' ';
          continue;
        case type::ending: //
          ++i;
          continue;
      }
    } else { // level > 0
      switch( t ) {
        case type::normal: //
          text[i] = ' ';
          continue;
        case type::starting: //
          ++level;
          text[i] = ' ';
          continue;
        case type::ending: //
          --level;
          text[i] = ' ';
          ++i;
          text[i] = ' ';
          continue;
      }
    }
  }
  // We only have one character left, so if we are in a comment
  // then we don't have enough characters to get out of it, so it
  // must be an error.
  if( level > 0 ) return base::invalid( i );
  return base::valid;
}

base::valid_or<int> blankify_line_comments( std::string& text ) {
  bool in_comment = false;
  int  i          = 0;
  for( ; i < int( text.size() ); ++i ) {
    char c = text[i];
    if( c == '\n' || c == '\r' )
      in_comment = false;
    else if( c == '#' ) {
      in_comment = true;
    }
    if( in_comment ) text[i] = ' ';
  }
  return base::valid;
}

base::valid_or<int> blankify_comments( std::string& text ) {
  // Order matters here.
  HAS_VALUE_OR_RET( blankify_nested_comments( text ) );
  HAS_VALUE_OR_RET( blankify_line_comments( text ) );
  return base::valid;
}

doc parse_file( string_view filename ) {
  auto buffer = base::read_text_file_as_string( filename );
  if( !buffer )
    FATAL( "{}", base::error_read_text_file_msg(
                     filename, buffer.error() ) );
  base::valid_or<int> v = blankify_comments( *buffer );
  if( !v ) {
    auto   ep = parz::ErrorPos::from_index( *buffer, v.error() );
    string msg = fmt::format(
        "{}:error:{}:{}: {}\n", filename, ep.line, ep.col,
        "incorrect/missing comment delimiter." );
    FATAL( "{}", msg );
  }
  UNWRAP_CHECK(
      d, parz::parse_from_string<doc>( filename, *buffer ) );
  return std::move( d );
}

} // namespace cl
