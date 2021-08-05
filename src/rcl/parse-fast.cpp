/****************************************************************
**parse-fast.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-04.
*
* Description: Fast parser for the rcl config language.
*
*****************************************************************/
#include "parse-fast.hpp"

// parz
#include "parz/error.hpp"

// base
#include "base/conv.hpp"
#include "base/error.hpp"

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
  *out = string_view( start, g_cur );
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
bool parse_key_val( vector<key_val>* out );

bool parse_table( table* out ) {
  DCHECK( g_cur != g_end );
  DCHECK( *g_cur == '{' );
  ++g_cur;

  vector<key_val> kvs;
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

bool parse_number( number* out ) {
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
  string_view sv( start, g_cur );

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
  *out = string_view( start, g_cur );
  return true;
}

bool parse_string( string_val* out ) {
  if( g_cur == g_end ) return false;

  // double-quoted string.
  if( *g_cur == '"' ) {
    ++g_cur;
    char const* start = g_cur;
    while( g_cur != g_end && *g_cur != '"' ) ++g_cur;
    if( g_cur == g_end ) return false;
    *out = string_val( string_view( start, g_cur ) );
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
    *out = string_val( string_view( start, g_cur ) );
    DCHECK( *g_cur == '\'' );
    ++g_cur;
    return true;
  }

  // unquoted string. End at end of line.
  string_view s;
  if( !parse_unquoted_string( &s ) ) return false;
  *out = string_val( s );
  return true;
}

bool look_ahead_str( string_view sv ) {
  if( int( g_end - g_cur ) < int( sv.size() ) ) return false;
  return string_view( g_cur, g_cur + sv.size() ) == sv;
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
    number n;
    if( !parse_number( &n ) ) return false;
    *out = value( n );
    return true;
  }

  // bool (true)
  if( look_ahead_str( "true" ) ) {
    g_cur += 4; // length of "true"
    *out = value{ boolean{ true } };
    return true;
  }

  // bool (false)
  if( look_ahead_str( "false" ) ) {
    g_cur += 5; // length of "false"
    *out = value{ boolean{ false } };
    return true;
  }

  // assume string
  string_val s;
  if( !parse_string( &s ) ) return false;
  *out = value{ std::move( s ) };
  return true;
}

bool parse_key_val( vector<key_val>* out ) {
  eat_blanks();
  string_view key;
  if( !parse_key( &key ) ) return false;
  if( !parse_assignment() ) return false;
  value v;
  if( !parse_value( &v ) ) return false;
  eat_blanks();
  // optional comma.
  if( g_cur != g_end && *g_cur == ',' ) ++g_cur;
  out->emplace_back( string( key ), std::move( v ) );
  return true;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
base::expect<rawdoc, string> parse_fast( string_view filename,
                                         string_view in ) {
  g_start = in.begin();
  g_cur   = g_start;
  g_end   = in.end();

  vector<key_val> kvs;
  while( parse_key_val( &kvs ) ) {}

  auto ep = parz::ErrorPos::from_index( in, g_cur - g_start );
  if( g_cur != g_end )
    return fmt::format( "{}:error:{}:{}: unexpected character\n",
                        filename, ep.line, ep.col );

  return rawdoc( table( std::move( kvs ) ) );
}

} // namespace rcl
