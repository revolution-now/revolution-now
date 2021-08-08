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

using namespace std;

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

bool is_leading_identifier_char( char c ) {
  return ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ) ||
         ( c == '_' );
}

bool is_identifier_char( char c ) {
  return ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'Z' ) ||
         ( c >= 'a' && c <= 'z' ) || ( c == '_' );
}

bool is_digit( char c ) { return ( c >= '0' && c <= '9' ); }

/****************************************************************
** Parsers
*****************************************************************/
void eat_blanks() {
  while( g_cur != g_end && is_blank( *g_cur ) ) ++g_cur;
}

bool parse_key( string_view* out ) {
  eat_blanks();
  if( g_cur == g_end ) return false;
  char const* start = g_cur;
  if( !is_leading_identifier_char( *start ) ) return false;
  bool got_dot = false;
  while( g_cur != g_end &&
         ( is_identifier_char( *g_cur ) || *g_cur == '.' ) ) {
    // Ensure we don't get two dots in a row.
    if( *g_cur == '.' ) {
      if( got_dot ) return false;
      got_dot = true;
    } else {
      got_dot = false;
    }
    ++g_cur;
  }
  *out = string_view( start, g_cur - start );
  return true;
}

bool parse_assignment() {
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
bool parse_key_val( vector<table::key_val>* out );

bool parse_table( table* out ) {
  DCHECK( g_cur != g_end );
  DCHECK( *g_cur == '{' );
  ++g_cur;

  vector<table::key_val> kvs;
  while( parse_key_val( &kvs ) )
    ;

  eat_blanks();
  if( g_cur == g_end || *g_cur != '}' ) return false;
  ++g_cur;

  *out = table( std::move( kvs ) );
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
  while( g_cur != g_end && *g_cur != '\n' && *g_cur != '\r' ) {
    if( *g_cur == '{' || *g_cur == '}' || *g_cur == '[' ||
        *g_cur == ']' || *g_cur == ',' || *g_cur == '"' ||
        *g_cur == '=' || *g_cur == ':' || *g_cur == '\'' )
      break;
    ++g_cur;
  }
  if( start == g_cur ) return false;
  // Eat trailing spaces. This is so that an unquoted string
  // won't e.g. include the space between the end of a word and a
  // closing brace of a table that is on the same line.
  // +1 because We know that the first character is not a blank.
  while( g_cur > start + 1 ) {
    if( is_blank( *( g_cur - 1 ) ) )
      --g_cur;
    else
      break;
  }
  *out = string_view( start, g_cur - start );
  return true;
}

bool parse_string( string* out ) {
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
  string_view s;
  if( !parse_unquoted_string( &s ) ) return false;
  *out = string( s );
  return true;
}

bool look_ahead_str( string_view sv ) {
  if( int( g_end - g_cur ) < int( sv.size() ) ) return false;
  return string_view( g_cur, sv.size() ) == sv;
}

bool parse_value( value* out ) {
  eat_blanks();
  if( g_cur == g_end ) return false;

  // table
  if( *g_cur == '{' ) {
    table tbl;
    if( !parse_table( &tbl ) ) return false;
    *out = value( make_unique<table>( std::move( tbl ) ) );
    return true;
  }

  // list
  if( *g_cur == '[' ) {
    list lst;
    if( !parse_list( &lst ) ) return false;
    *out = value( make_unique<list>( std::move( lst ) ) );
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
  if( !parse_string( &s ) ) return false;

  // Intercept bool (true)
  if( s == "true" ) {
    *out = value{ true };
    return true;
  }

  // Intercept bool (false)
  if( look_ahead_str( "false" ) ) {
    *out = value{ false };
    return true;
  }

  *out = value{ std::move( s ) };
  return true;
}

bool parse_key_val( vector<table::key_val>* out ) {
  eat_blanks();
  string_view key;
  if( !parse_key( &key ) ) return false;
  if( !parse_assignment() ) return false;
  value v;
  if( !parse_value( &v ) ) return false;
  eat_blanks();
  // optional comma.
  if( g_cur != g_end && *g_cur == ',' ) ++g_cur;
  out->push_back( { string( key ), std::move( v ) } );
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

} // namespace

/****************************************************************
** Public API
*****************************************************************/
base::expect<doc, string> parse( string_view filename,
                                 string_view in ) {
  g_start = in.begin();
  g_cur   = g_start;
  g_end   = in.end();

  vector<table::key_val> kvs;
  while( parse_key_val( &kvs ) ) {}

  auto [line, col] = error_pos( in, g_cur - g_start );
  if( g_cur != g_end )
    return fmt::format( "{}:error:{}:{}: unexpected character\n",
                        filename, line, col );

  return doc::create( table( std::move( kvs ) ) );
}

base::expect<doc, string> parse_file( string_view filename ) {
  auto buffer = base::read_text_file_as_string( filename );
  if( !buffer )
    FATAL( "{}", base::error_read_text_file_msg(
                     filename, buffer.error() ) );
  blankify_comments( *buffer );
  return parse( filename, *buffer );
}

} // namespace rcl
