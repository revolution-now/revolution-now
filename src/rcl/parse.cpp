/****************************************************************
**parse.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-04.
*
* Description: Parser for the rcl config language.
*
*****************************************************************/
#include "parse.hpp"

// base
#include "base/conv.hpp"
#include "base/error.hpp"
#include "base/io.hpp"

// C++ standard library
#include <cassert>

using namespace std;

using ::cdr::list;
using ::cdr::table;
using ::cdr::value;

namespace rcl {

namespace {

#define FAIL_RESTORE ( ( g_cur = sav ), false )

/****************************************************************
** Global State
*****************************************************************/
char const* g_start = nullptr;
char const* g_cur   = nullptr;
char const* g_end   = nullptr;

/****************************************************************
** Helpers
*****************************************************************/
// Given a multiline string and a position in it, compute the
// line and column numbers for error messages.
pair<int, int> error_pos( string_view in, int idx ) {
  assert( idx <= int( in.size() ) );
  int line = 1, col = 1;
  for( int i = 0; i < idx; ++i ) {
    ++col;
    if( in[i] == '\n' ) {
      ++line;
      col = 1;
    }
  }
  return { line, col };
}

bool is_nonnewline_blank( char c ) {
  return ( c == ' ' ) || ( c == '\t' );
}

bool is_blank( char c ) {
  return ( c == ' ' ) || ( c == '\n' ) || ( c == '\r' ) ||
         ( c == '\t' );
}

bool is_newline( char c ) {
  return ( c == '\n' ) || ( c == '\r' );
}

bool is_digit( char c ) { return ( c >= '0' && c <= '9' ); }

bool is_alpha( char c ) {
  return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
}

// This will remove trailing spaces from the end and will also
// move the global cursor back as well.
void trim_and_back_up( string_view* out ) {
  // Remove trailing spaces.
  while( !out->empty() &&
         is_blank( ( *out )[out->size() - 1] ) ) {
    out->remove_suffix( 1 );
    --g_cur;
  }
}

/****************************************************************
** Parsers
*****************************************************************/
void eat_blanks() {
  while( g_cur != g_end && is_blank( *g_cur ) ) ++g_cur;
}

// A table key can be a space and/or dot-separated list of compo-
// nents, which a "component" can either be an identifier or an
// arbitrary string in double quotes. Any characters inside the
// double quotes must have any literal double quote characters
// escaped with a backslash, and must have any literal back-
// slashes escaped with a backslash.
//
// This function will parse a key and make sure that it is valid,
// but will not transform it in any way (that is done by the
// model post-processor).
bool parse_key( string_view* out ) {
  eat_blanks();
  if( g_cur == g_end ) return false;
  char const* start = g_cur;
  if( !is_leading_identifier_char( *start ) && *start != '"' )
    return false;

  bool got_dot  = false;
  bool in_quote = false;
  // This allows a series of identifiers separated by dots and/or
  // spaces (which are equivalent), potentially with quotes to
  // allow spaces and weird characters inside a key.
  while( g_cur != g_end ) {
    if( in_quote ) {
      // We are in a quote.
      CHECK( !got_dot );
      if( *g_cur == '\\' ) {
        // We're escaping something, so we must have a next char-
        // acter in the stream, since a single backslash inside a
        // quote is not valid.
        ++g_cur;
        if( g_cur == g_end ) return false;
        // Accept whatever the next character is.
        ++g_cur;
        continue;
      }
      // We're not escaping anything.
      if( *g_cur == '"' ) {
        // This quote is being closed.
        in_quote = false;
      } else if( is_newline( *g_cur ) ) {
        // Unclosed quote... fail.
        return false;
      }
      // Any other char: accept it.
      ++g_cur;
      continue;
    }

    // We're not in a quote, so now check if we're opening one.
    if( *g_cur == '"' ) {
      // We are opening a quote.
      CHECK( !in_quote );
      in_quote = true;
      ++g_cur;
      got_dot = false;
      continue;
    }

    // We are not in a quote and not opening one, so now we have
    // some restrictions on allowed chars; actually, if we get an
    // unallowed char, we assume that is the end of the key (not
    // an error).
    if( !is_identifier_char( *g_cur ) && *g_cur != '.' &&
        *g_cur != ' ' )
      break;

    // Ensure we don't get two dots in a row, even if they have
    // spaces between them.
    if( *g_cur == '.' ) {
      if( got_dot ) return false;
      got_dot = true;
    } else if( !is_blank( *g_cur ) ) {
      got_dot = false;
    }
    ++g_cur;
  }
  *out = string_view( start, g_cur - start );
  trim_and_back_up( out );
  return true;
}

bool parse_assignment() {
  if( g_cur != g_end && *g_cur == '{' ) return true;
  bool has_space = false;
  while( g_cur != g_end && is_nonnewline_blank( *g_cur ) ) {
    ++g_cur;
    has_space = true;
  }
  if( g_cur == g_end ) return false;
  if( *g_cur == '=' || *g_cur == ':' ) {
    ++g_cur;
    return true;
  }
  return has_space;
}

bool parse_value( value* out );
bool parse_key_val( table* out );

bool parse_table( table* out ) {
  DCHECK( g_cur != g_end );
  DCHECK( *g_cur == '{' );
  ++g_cur;

  table tbl;
  while( true ) {
    eat_blanks();
    char const* sav = g_cur;
    bool success    = parse_key_val( &tbl );
    if( !success ) {
      if( g_cur != sav )
        // We failed but parsed some non-blank characters,
        // meaning that there was a syntax error.
        return false;
      break;
    }
  }

  eat_blanks();
  if( g_cur == g_end || *g_cur != '}' ) return false;
  ++g_cur;

  *out = std::move( tbl );
  return true;
}

bool parse_list( list* out ) {
  DCHECK( g_cur != g_end );
  DCHECK( *g_cur == '[' );
  ++g_cur;

  vector<value> vs;
  while( true ) {
    value v;
    if( !parse_value( &v ) ) break;
    eat_blanks();
    // optional comma.
    if( g_cur != g_end && *g_cur == ',' ) ++g_cur;
    vs.push_back( std::move( v ) );
  }

  eat_blanks();
  if( g_cur == g_end || *g_cur != ']' ) return false;
  ++g_cur;

  *out = list( std::move( vs ) );
  return true;
}

bool parse_number( value* out ) {
  char const* sav = g_cur;
  DCHECK( g_cur != g_end );
  char const* start = g_cur;
  while( g_cur != g_end && ( is_digit( *g_cur ) ||
                             *g_cur == '-' || *g_cur == '.' ) )
    ++g_cur;
  if( g_cur == start ) return FAIL_RESTORE;
  if( g_cur != g_end ) {
    // make sure we have a word boundary. This basically means
    // that we have something that a) is not a digit (which we
    // already know it isn't), and b) is not the start of an
    // identifier.
    if( is_leading_identifier_char( *g_cur ) )
      return FAIL_RESTORE;
  }
  string_view sv( start, g_cur - start );

  if( sv.find_first_of( '.' ) != string_view::npos ) {
    // double.
    base::maybe<double> d = base::stod( string( sv ) );
    if( !d ) return FAIL_RESTORE;
    *out = *d;
    return true;
  } else {
    // int.
    base::maybe<int> i = base::stoi( string( sv ) );
    if( !i ) { return FAIL_RESTORE; }
    *out = *i;
    return true;
  }
}

bool parse_unquoted_string( string_view* out ) {
  char const* start = g_cur;
  while( g_cur != g_end ) {
    if( is_forbidden_unquoted_str_char( *g_cur ) ) break;
    ++g_cur;
  }
  if( start == g_cur ) return false;
  // Eat trailing spaces. This is so that an unquoted string
  // won't e.g. include the space between the end of a word and a
  // closing brace of a table that is on the same line. +1 be-
  // cause We know that the first character is not a blank.
  while( g_cur > start + 1 ) {
    if( is_blank( *( g_cur - 1 ) ) )
      --g_cur;
    else
      break;
  }
  *out = string_view( start, g_cur - start );
  return true;
}

bool parse_string( string* out, bool* unquoted ) {
  *unquoted = false;
  if( g_cur == g_end ) return false;

  // double-quoted string.
  if( *g_cur == '"' ) {
    ++g_cur;
    char const* start = g_cur;
    while( g_cur != g_end && *g_cur != '"' ) ++g_cur;
    if( g_cur == g_end ) return false;
    *out = string( start, g_cur );
    DCHECK( *g_cur == '"' );
    ++g_cur;
    return true;
  }

  // single-quoted string.
  if( *g_cur == '\'' ) {
    ++g_cur;
    char const* start = g_cur;
    while( g_cur != g_end && *g_cur != '\'' ) ++g_cur;
    if( g_cur == g_end ) return false;
    *out = string( start, g_cur );
    DCHECK( *g_cur == '\'' );
    ++g_cur;
    return true;
  }

  // unquoted string. End at end of line.
  if( is_forbidden_leading_unquoted_str_char( *g_cur ) )
    return false;
  *unquoted = true;
  string_view s;
  if( !parse_unquoted_string( &s ) ) return false;
  *out = string( s );
  return true;
}

bool parse_value( value* out ) {
  eat_blanks();
  if( g_cur == g_end ) return false;

  // table
  if( *g_cur == '{' ) {
    table tbl;
    if( !parse_table( &tbl ) ) return false;
    *out = value( std::move( tbl ) );
    return true;
  }

  // implicit table.
  if( g_cur + 1 < g_end && *g_cur == '.' &&
      is_alpha( *( g_cur + 1 ) ) ) {
    table tbl;
    ++g_cur;
    if( !parse_key_val( &tbl ) ) return false;
    *out = value( std::move( tbl ) );
    return true;
  }

  // list
  if( *g_cur == '[' ) {
    list lst;
    if( !parse_list( &lst ) ) return false;
    *out = value( std::move( lst ) );
    return true;
  }

  // number
  if( *g_cur == '-' || *g_cur == '.' || is_digit( *g_cur ) ) {
    value v;
    if( !parse_number( &v ) ) return false;
    *out = std::move( v );
    return true;
  }

  // Assume string.
  string s;
  bool unquoted;
  if( !parse_string( &s, &unquoted ) ) return false;

  if( unquoted ) {
    // Intercept null
    if( s == "null" ) {
      *out = value{ cdr::null };
      return true;
    }

    // Intercept bool (true)
    if( s == "true" ) {
      *out = value{ true };
      return true;
    }

    // Intercept bool (false)
    if( s == "false" ) {
      *out = value{ false };
      return true;
    }
  }

  *out = value{ std::move( s ) };
  return true;
}

bool parse_key_val( table* out ) {
  eat_blanks();
  string_view key;
  if( !parse_key( &key ) ) return false;
  if( out->contains( string( key ) ) ) return false;
  if( !parse_assignment() ) return false;
  value v;
  if( !parse_value( &v ) ) return false;
  eat_blanks();
  // optional comma.
  if( g_cur != g_end && *g_cur == ',' ) ++g_cur;
  out->emplace( string( key ), std::move( v ) );
  return true;
}

/****************************************************************
** Comments Blankifier
*****************************************************************/
// This will keep the string the same length but will ovewrite
// all comments (comment delimiters and comment contents) with
// spaces).
void blankify_comments( string& text ) {
  bool in_comment     = false;
  bool in_dquoted_str = false;
  bool in_squoted_str = false;
  int i               = 0;
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

} // namespace

/****************************************************************
** Public API
*****************************************************************/
base::expect<doc, string> parse(
    string_view filename, string const& in_with_comments,
    ProcessingOptions const& opts ) {
  string in_blankified = in_with_comments;
  blankify_comments( in_blankified );
  string_view in = in_blankified;
  g_start        = in.begin();
  g_cur          = g_start;
  g_end          = in.end();

  table tbl;
  // At the top level, the document must be a table. However,
  // top-level braces are optional (unlike with JSON). So first
  // check if we are parsing a JSON-like document and, if not,
  // fall back to just parsing the key/value pairs directly.
  eat_blanks();
  if( g_cur != g_end && *g_cur == '{' )
    parse_table( &tbl );
  else
    while( parse_key_val( &tbl ) ) {}
  eat_blanks();

  auto [line, col] = error_pos( in, g_cur - g_start );
  if( g_cur != g_end )
    return fmt::format( "{}:error:{}:{}: unexpected character",
                        filename, line, col );

  return doc::create( std::move( tbl ), opts );
}

base::expect<doc> parse_file( string_view filename,
                              ProcessingOptions const& opts ) {
  auto buffer = base::read_text_file_as_string( filename );
  if( !buffer )
    FATAL( "{}", base::error_read_text_file_msg(
                     filename, buffer.error() ) );
  return parse( filename, *buffer, opts );
}

} // namespace rcl
