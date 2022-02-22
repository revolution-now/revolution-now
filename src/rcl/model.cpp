/****************************************************************
**model.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Document model for rcl (config language) files.
*
*****************************************************************/
#include "model.hpp"

// rcl
#include "emit.hpp"

// cdr
#include "cdr/converter.hpp"

// base
#include "base/macros.hpp" // FIXME: remove

// base-util
#include "base-util/stopwatch.hpp"

// C++ standard library
#include <sstream>

using namespace std;

namespace rcl {

using ::base::expect;
using ::base::maybe;
using ::base::nothing;
using ::cdr::list;
using ::cdr::null_t;
using ::cdr::table;
using ::cdr::value;

namespace {

template<typename F, typename V>
auto fast_visit( F&& f, V&& v ) {
  switch( v.index() ) {
    case 0: {
      auto& o = v.template get<null_t>();
      return f( std::move( o ) );
    }
    case 1: {
      auto& o = v.template get<double>();
      return f( std::move( o ) );
    }
    case 2: {
      auto& o = v.template get<cdr::integer_type>();
      return f( std::move( o ) );
    }
    case 3: {
      auto& o = v.template get<bool>();
      return f( std::move( o ) );
    }
    case 4: {
      auto& o = v.template get<std::string>();
      return f( std::move( o ) );
    }
    case 5: {
      auto& o = v.template get<table>();
      return f( std::move( o ) );
    }
    case 6: {
      auto& o = v.template get<list>();
      return f( std::move( o ) );
    }
  }
  UNREACHABLE_LOCATION;
}

template<typename F, typename V>
auto fast_visit_lvalue( F&& f, V&& v ) {
  switch( v.index() ) {
    case 0: {
      auto& o = v.template get<null_t>();
      return f( o );
    }
    case 1: {
      auto& o = v.template get<double>();
      return f( o );
    }
    case 2: {
      auto& o = v.template get<cdr::integer_type>();
      return f( o );
    }
    case 3: {
      auto& o = v.template get<bool>();
      return f( o );
    }
    case 4: {
      auto& o = v.template get<std::string>();
      return f( o );
    }
    case 5: {
      auto& o = v.template get<table>();
      return f( o );
    }
    case 6: {
      auto& o = v.template get<list>();
      return f( o );
    }
  }
  UNREACHABLE_LOCATION;
}

} // namespace

/****************************************************************
** Formatting
*****************************************************************/
bool is_leading_identifier_char( char c ) {
  return ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ) ||
         ( c == '_' );
}

bool is_identifier_char( char c ) {
  return ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'Z' ) ||
         ( c >= 'a' && c <= 'z' ) || ( c == '_' );
}

// This needs to be a subset of the corresponding "leading char"
// function below.
bool is_forbidden_unquoted_str_char( char c ) {
  static char const forbidden[] = "\n\r{}\\[],\"=:'";
  for( char forbidden_char : forbidden )
    if( c == forbidden_char ) return true;
  return false;
}

bool is_forbidden_leading_unquoted_str_char( char c ) {
  static char const forbidden[] = "\n\r{}\\[],\"=:'-0123456789.";
  for( char forbidden_char : forbidden )
    if( c == forbidden_char ) return true;
  return false;
}

string escape_and_quote_table_key( string const& k ) {
  if( k.empty() ) return k;
  bool need_quotes = false;
  // We have a stronger requirement for the first character.
  if( !is_leading_identifier_char( k[0] ) ) need_quotes = true;
  string res;
  for( char c : k ) {
    if( !is_identifier_char( c ) ) need_quotes = true;
    if( c == '"' ) res += '\\';
    if( c == '\\' ) res += '\\';
    res += c;
  }
  if( need_quotes ) res = "\""s + res + "\""s;
  return res;
};

string escape_and_quote_string_val( string const& k ) {
  if( k.empty() ) return k;
  bool need_quotes = false;
  // We have a stronger requirement for the first character.
  if( is_forbidden_leading_unquoted_str_char( k[0] ) )
    need_quotes = true;
  // If it starts with these then the parser would confuse it
  // with a boolean.
  if( k.starts_with( "true" ) || k.starts_with( "false" ) )
    need_quotes = true;
  if( k == "null" ) need_quotes = true;
  string res;
  for( char c : k ) {
    if( is_forbidden_unquoted_str_char( c ) ) need_quotes = true;
    if( c == '"' ) res += '\\';
    if( c == '\\' ) res += '\\';
    res += c;
  }
  if( need_quotes ) res = "\""s + res + "\""s;
  return res;
}

/****************************************************************
** doc
*****************************************************************/
void to_str( doc const& o, string& out, base::ADL_t ) {
  out += emit( o, EmitOptions{ .flatten_keys = false } );
}

/****************************************************************
** Table Key Parser
*****************************************************************/
// A raw key string can consist of multiple sequential keys, and
// these will eventually be unflattened into nested tables. Fur-
// thermore, each key within the raw key string can be quoted,
// and the characters inside the quotes can be escaped. There-
// fore, the raw key strings are non-trivial to parse and to
// split into real keys. This post-processing step will do that
// parsing, and will yield a new key string with 0x01 chars to
// delimit real keys. Contiguous chunks of spaces and dots (both
// outside of quotes) can also separate real keys, and so those
// will be replaced with 0x01 as well.
namespace {

table key_parser_table( table&& in );
list  key_parser_list( list&& in );

constexpr char        kKeyDelimiterChar = 0x01;
constexpr string_view kKeyDelimiterStr  = "\001";
static_assert( kKeyDelimiterStr[0] == kKeyDelimiterChar );

struct key_parser_visitor {
  value operator()( null_t ) const { return value{ cdr::null }; }

  value operator()( bool o ) const { return value{ o }; }

  value operator()( cdr::integer_type n ) const {
    return value{ n };
  }

  value operator()( double d ) const { return value{ d }; }

  value operator()( std::string&& o ) const {
    return value{ std::move( o ) };
  }

  value operator()( table&& o ) const {
    return value{ key_parser_table( std::move( o ) ) };
  }

  value operator()( list&& o ) const {
    return value{ key_parser_list( std::move( o ) ) };
  }
};

void key_parser_impl( table& in, string&& raw_key, value&& v ) {
  string res;
  if( raw_key.size() > res.capacity() )
    res.reserve( raw_key.size() * 2 );
  bool in_quote = false;
  auto cur      = raw_key.begin();
  auto end      = raw_key.end();
  while( cur != end ) {
    if( in_quote ) {
      // We are in a quote.
      if( *cur == '\\' ) {
        // We're escaping something. If this is followed by one
        // of the special escape chars, then action it, otherwise
        // accept whatever we get literally, including the back-
        // slash.
        if( cur + 1 == end ) {
          res += '\\';
          break;
        }
        // We have a next char.
        ++cur;
        if( *cur == '\\' ) {
          res += '\\';
          ++cur;
          continue;
        }
        if( *cur == '"' ) {
          res += '"';
          ++cur;
          continue;
        }
        // The Rcl parser should not allow this case, but we
        // allow it here because we could be getting input from
        // sources other than parsed Rcl (e.g., from Cdr).
        res += '\\';
        res += *cur;
        ++cur;
        continue;
      }
      // We're not escaping anything.
      if( *cur == '"' ) {
        // This quote is being closed.
        in_quote = false;
        ++cur;
        continue;
      }
      // Any other char: accept it.
      res += *cur;
      ++cur;
      continue;
    }

    // We're not in a quote, so now check if we're opening one.
    if( *cur == '"' ) {
      // We are opening a quote.
      CHECK( !in_quote );
      in_quote = true;
      ++cur;
      continue;
    }

    // We are not in a quote and not opening one, so now we have
    // some special interpretations of characters, namely a dot
    // or consecutive spaces can delimit keys.
    if( *cur == ' ' || *cur == '.' ) {
      // Consolodate consecutive spaces/dots into one delimiter.
      while( cur != end && ( *cur == ' ' || *cur == '.' ) )
        ++cur;
      if( res.size() == 0 || res.back() != kKeyDelimiterChar )
        res += kKeyDelimiterChar;
      continue;
    }

    // Non-special char, just accept it.
    res += *cur;
    ++cur;
  }
  in.emplace( std::move( res ), fast_visit( key_parser_visitor{},
                                            std::move( v ) ) );
}

table key_parser_table( table&& in ) {
  table t;
  for( auto& [k, v] : in )
    key_parser_impl( t, string( k ), std::move( v ) );
  return t;
}

list key_parser_list( list&& in ) {
  list l;
  l.reserve( in.size() );
  for( value& v : in )
    l.push_back(
        fast_visit( key_parser_visitor{}, std::move( v ) ) );
  return l;
}

} // namespace

/****************************************************************
** Table Unflattening
*****************************************************************/
namespace {

table unflatten_table( table&& in );
list  unflatten_list( list&& in );

struct unflatten_visitor {
  value operator()( null_t ) const { return value{ cdr::null }; }

  value operator()( bool o ) const { return value{ o }; }

  value operator()( cdr::integer_type n ) const {
    return value{ n };
  }

  value operator()( double d ) const { return value{ d }; }

  value operator()( std::string&& o ) const {
    return value{ std::move( o ) };
  }

  value operator()( table&& o ) const {
    return value{ unflatten_table( std::move( o ) ) };
  }

  value operator()( list&& o ) const {
    return value{ unflatten_list( std::move( o ) ) };
  }
};

void unflatten_impl( table& in, string&& dotted, value&& v ) {
  int i = dotted.find_first_of( kKeyDelimiterStr );
  if( i == int( string_view::npos ) ) {
    in.emplace(
        std::move( dotted ),
        fast_visit( unflatten_visitor{}, std::move( v ) ) );
    return;
  }
  DCHECK( i + 1 < int( dotted.size() ) );
  string_view key  = string_view( dotted ).substr( 0, i );
  string_view rest = string_view( dotted ).substr( i + 1 );
  auto [it, b] = in.emplace( string( key ), value{ table{} } );
  table& tbl   = it->second.get<table>();
  unflatten_impl( tbl, string( rest ), std::move( v ) );
}

table unflatten_table( table&& in ) {
  table t;
  for( auto& [k, v] : in )
    unflatten_impl( t, string( k ), std::move( v ) );
  return t;
}

list unflatten_list( list&& in ) {
  static const unflatten_visitor vis;
  list                           l;
  l.reserve( in.size() );
  for( value& v : in )
    l.push_back( fast_visit( vis, std::move( v ) ) );
  return l;
}

} // namespace

namespace {

/****************************************************************
** Post-processing Routine
*****************************************************************/
// This should only be run on the top-level table, since it will
// be applied recursively. In practice, this is only really
// useful for unit tests, since otherwise the top-level table is
// placed into a doc, and the doc will run this post-processing.
base::expect<table, string> run_postprocessing(
    table&& v1, ProcessingOptions const& opts ) {
  util::StopWatch watch;
  table           v2;
  if( opts.run_key_parse ) {
    watch.start( "[post-processing] key parser" );
    v2 = key_parser_table( std::move( v1 ) );
    watch.stop( "[post-processing] key parser" );
  } else {
    v2 = std::move( v1 );
  }
  table v3;
  if( opts.unflatten_keys ) {
    watch.start( "[post-processing] unflatten" );
    v3 = unflatten_table( std::move( v2 ) );
    watch.stop( "[post-processing] unflatten" );
  } else {
    v3 = std::move( v2 );
  }
  table v4 = std::move( v3 );
#if 0
  for( auto const& p : watch.results() )
    fmt::print( "{}: {}\n", p.first, p.second );
#endif
  return v4;
}

} // namespace

/****************************************************************
** Document
*****************************************************************/
expect<doc> doc::create( table&&                  tbl,
                         ProcessingOptions const& opts ) {
  UNWRAP_RETURN( postprocessed,
                 run_postprocessing( std::move( tbl ), opts ) );
  return doc( std::move( postprocessed ) );
}

} // namespace rcl
