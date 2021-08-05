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

void blankify_comments( std::string& text ) {
  bool in_comment     = false;
  bool in_dquoted_str = false;
  bool in_squoted_str = false;
  int  i              = 0;
  for( ; i < int( text.size() ); ++i ) {
    char c = text[i];
    if( c == '\n' || c == '\r' ) {
      in_comment     = false;
      in_dquoted_str = false;
      in_squoted_str = false;
      continue;
    }
    if( in_dquoted_str ) {
      if( c != '"' ) continue;
      in_dquoted_str = false;
      continue;
    }
    if( in_squoted_str ) {
      if( c != '\'' ) continue;
      in_squoted_str = false;
      continue;
    }
    if( in_comment ) {
      text[i] = ' ';
      continue;
    }
    // We're not in a comment or string.
    if( c == '#' ) {
      // start comment.
      in_comment = true;
      text[i]    = ' ';
      continue;
    }
    if( c == '"' ) {
      // start double quoted string[
      in_dquoted_str = true;
      continue;
    }
    if( c == '\'' ) {
      // start single quoted string[
      in_squoted_str = true;
      continue;
    }
  }
}

base::expect<doc, std::string> parse_file(
    string_view filename ) {
  auto buffer = base::read_text_file_as_string( filename );
  if( !buffer )
    FATAL( "{}", base::error_read_text_file_msg(
                     filename, buffer.error() ) );
  blankify_comments( *buffer );
  UNWRAP_RETURN( rdoc, parz::parse_from_string<cl_lang, rawdoc>(
                           filename, *buffer ) );
  return doc::create( std::move( rdoc ) );
}

} // namespace cl
