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
}

doc parse_file( string_view filename ) {
  auto buffer = base::read_text_file_as_string( filename );
  if( !buffer )
    FATAL( "{}", base::error_read_text_file_msg(
                     filename, buffer.error() ) );
  blankify_comments( *buffer );
  UNWRAP_CHECK( d, ( parz::parse_from_string<cl_lang, doc>(
                       filename, *buffer ) ) );
  return std::move( d );
}

} // namespace cl
