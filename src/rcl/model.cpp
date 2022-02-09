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

// base
#include "base/valid.hpp" // FIXME: remove

// Abseil
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"

// C++ standard library
#include <sstream>
#include <unordered_map>

using namespace std;

namespace rcl {

using ::base::expect;
using ::base::maybe;
using ::base::nothing;
using ::base::valid;
using ::base::valid_or;

/****************************************************************
** Formatting
*****************************************************************/
struct value_printer {
  string_view indent;

  string operator()( null_t ) const { return "null"; }

  string operator()( bool b ) const {
    return b ? "true" : "false";
  }

  string operator()( int n ) const {
    return fmt::to_string( n );
  }

  string operator()( double d ) const {
    return fmt::to_string( d );
  }

  string operator()( string const& s ) const {
    return fmt::format( "\"{}\"", s );
  }

  string operator()( std::unique_ptr<table> const& tbl ) const {
    return tbl->pretty_print( indent );
  }

  string operator()( std::unique_ptr<list> const& lst ) const {
    return lst->pretty_print( indent );
  }
};

bool is_leading_identifier_char( char c ) {
  return ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ) ||
         ( c == '_' );
}

bool is_identifier_char( char c ) {
  return ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'Z' ) ||
         ( c >= 'a' && c <= 'z' ) || ( c == '_' );
}

/****************************************************************
** value
*****************************************************************/
type type_of( value const& v ) {
  return std::visit( type_visitor{}, v );
}

string_view name_of( type t ) {
  switch( t ) {
    case type::null: return "null";
    case type::boolean: return "bool";
    case type::integral: return "int";
    case type::floating: return "double";
    case type::string: return "string";
    case type::table: return "table";
    case type::list: return "list";
  }
}

/****************************************************************
** table
*****************************************************************/
bool table::has_key( std::string_view key ) const {
  return map_.contains( key );
}

value const& table::operator[]( string_view key ) const {
  auto it = map_.find( key );
  CHECK( it != map_.end(), "table does not contain key {}",
         key );
  value* p = it->second;
  DCHECK( p );
  return *p;
}

table::value_type const& table::operator[]( int n ) const {
  CHECK( n >= 0 && n < int( members_.size() ),
         "invalid table index {}", n );
  return members_[n];
}

namespace {

string format_key( string const& k ) {
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

} // namespace

string table::pretty_print( string_view indent ) const {
  bool          is_top_level = ( indent.size() == 0 );
  ostringstream oss;
  if( indent.size() > 0 ) oss << fmt::format( "{{\n" );

  size_t n = members_.size();
  for( auto& [k, v] : members_ ) {
    string assign = ":";
    if( v.holds<unique_ptr<table>>() ) assign = "";
    string k_str = format_key( k );
    oss << fmt::format(
        "{}{}{} {}\n", indent, k_str, assign,
        std::visit( value_printer{ string( indent ) + "  " },
                    v ) );
    if( is_top_level && n-- > 1 ) oss << "\n";
  }

  if( !is_top_level ) {
    indent.remove_suffix( 2 );
    oss << fmt::format( "{}}}", indent );
  }

  return oss.str();
}

/****************************************************************
** list
*****************************************************************/
value const& list::operator[]( int n ) const {
  CHECK( n >= 0 && n < int( members_.size() ),
         "invalid table index {}", n );
  return members_[n];
}

string list::pretty_print( string_view indent ) const {
  ostringstream oss;
  if( indent.size() > 0 ) oss << fmt::format( "[\n" );

  for( auto& v : members_ )
    oss << fmt::format(
        "{}{},\n", indent,
        std::visit( value_printer{ string( indent ) + "  " },
                    v ) );

  indent.remove_suffix( 2 );
  oss << fmt::format( "{}]", indent );

  return oss.str();
}

/****************************************************************
** doc
*****************************************************************/
string doc::pretty_print( string_view indent ) const {
  return tbl_->pretty_print( indent );
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

constexpr char        kKeyDelimiterChar = 0x01;
constexpr string_view kKeyDelimiterStr  = "\001";
static_assert( kKeyDelimiterStr[0] == kKeyDelimiterChar );

struct key_parser_visitor {
  value operator()( null_t ) const { return value{ null }; }

  value operator()( bool o ) const { return value{ o }; }

  value operator()( int n ) const { return value{ n }; }

  value operator()( double d ) const { return value{ d }; }

  value operator()( std::string&& o ) const {
    return value{ std::move( o ) };
  }

  value operator()( unique_ptr<table>&& o ) const {
    return value{
        make_unique<table>( std::move( *o ).key_parser() ) };
  }

  value operator()( unique_ptr<list>&& o ) const {
    return value{
        make_unique<list>( std::move( *o ).key_parser() ) };
  }
};

} // namespace

void table::key_parser_impl( string_view raw_key, value&& v ) {
  string res;
  bool   in_quote = false;
  auto   cur      = raw_key.begin();
  auto   end      = raw_key.end();
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
      res += kKeyDelimiterChar;
      continue;
    }

    // Non-special char, just accept it.
    res += *cur;
    ++cur;
  }
  vector<string> keys = absl::StrSplit( res, kKeyDelimiterStr );
  erase_if( keys, []( string const& s ) { return s.empty(); } );
  members_.emplace_back(
      absl::StrJoin( keys, kKeyDelimiterStr ),
      std::visit( key_parser_visitor{}, std::move( v ) ) );
}

table table::key_parser() && {
  table t;
  for( auto& [k, v] : members_ )
    t.key_parser_impl( k, std::move( v ) );
  return t;
}

list list::key_parser() && {
  list l;
  for( value& v : members_ )
    l.members_.push_back(
        std::visit( key_parser_visitor{}, std::move( v ) ) );
  return l;
}

/****************************************************************
** Table Unflattening
*****************************************************************/
namespace {

struct unflatten_visitor {
  value operator()( null_t ) const { return value{ null }; }

  value operator()( bool o ) const { return value{ o }; }

  value operator()( int n ) const { return value{ n }; }

  value operator()( double d ) const { return value{ d }; }

  value operator()( std::string&& o ) const {
    return value{ std::move( o ) };
  }

  value operator()( unique_ptr<table>&& o ) const {
    return value{
        make_unique<table>( std::move( *o ).unflatten() ) };
  }

  value operator()( unique_ptr<list>&& o ) const {
    return value{
        make_unique<list>( std::move( *o ).unflatten() ) };
  }
};

} // namespace

void table::unflatten_impl( string_view dotted, value&& v ) {
  int i = dotted.find_first_of( kKeyDelimiterStr );
  if( i == int( string_view::npos ) ) {
    members_.emplace_back(
        string( dotted ),
        std::visit( unflatten_visitor{}, std::move( v ) ) );
    return;
  }
  DCHECK( i + 1 < int( dotted.size() ) );
  string key  = string( dotted.substr( 0, i ) );
  string rest = string( dotted.substr( i + 1 ) );
  // Add a new one even if there is already a key with the same
  // name in the table, because our tables allow duplicate keys.
  pair<string, value>& new_kv = members_.emplace_back();
  new_kv.first                = key;
  new_kv.second               = value{ make_unique<table>() };
  table* tbl = new_kv.second.get<unique_ptr<table>>().get();
  tbl->unflatten_impl( rest, std::move( v ) );
}

table table::unflatten() && {
  table t;
  for( auto& [k, v] : members_ )
    t.unflatten_impl( k, std::move( v ) );
  return t;
}

list list::unflatten() && {
  list l;
  for( value& v : members_ )
    l.members_.push_back(
        std::visit( unflatten_visitor{}, std::move( v ) ) );
  return l;
}

/****************************************************************
** Table Key Deduplication
*****************************************************************/
namespace {

struct dedupe_visitor {
  expect<value, string> operator()( null_t ) const {
    return value{ null };
  }

  expect<value, string> operator()( bool b ) const {
    return value{ b };
  }

  expect<value, string> operator()( int n ) const {
    return value{ n };
  }

  expect<value, string> operator()( double d ) const {
    return value{ d };
  }

  expect<value, string> operator()( string&& o ) const {
    return value{ std::move( o ) };
  }

  expect<value, string> operator()(
      unique_ptr<table>&& o ) const {
    UNWRAP_RETURN( deduped, std::move( *o ).dedupe() );
    return value{ make_unique<table>( std::move( deduped ) ) };
  }

  expect<value, string> operator()(
      unique_ptr<list>&& o ) const {
    UNWRAP_RETURN( deduped, std::move( *o ).dedupe_tables() );
    return value{ make_unique<list>( std::move( deduped ) ) };
  }
};

} // namespace

// If the two values are both tables then they will be merged, in
// the sense that all of the source table's keys (and values)
// will be moved into the target table. Any duplicate keys that
// result in the target table will be handled by recursively ap-
// plying the deduplication/merge logic.
//
// If one or both of the values are not tables then this function
// returns an error. We don't check-fail in that case (or try to
// enforce it with types by making this function accept table pa-
// rameters only) because that situation can result from an in-
// valid config file supplied by the user, and so we need to be
// able to receive that sort of invalid input and handle it by
// producing a proper error message for the user.
//
// Note: target is an lvalue ref and the source is an rvalue ref.
valid_or<string> merge_values( string_view key, value& v_target,
                               value&& v_source ) {
  using T = unique_ptr<table>;
  // The two values can only be merged if they are both tables,
  // so make sure that they are and fail otherwise.
  maybe<T&> target_tbl = v_target.get_if<T>();
  maybe<T&> source_tbl = v_source.get_if<T>();
  if( !target_tbl || !source_tbl )
    return fmt::format(
        "key `{}' has a duplicate but not all of its values "
        "are tables.",
        key );
  DCHECK( *target_tbl );
  DCHECK( *source_tbl );

  // The two tables may have duplicate keys that need to be
  // merged, but we will just combine the tables anyway, then
  // dedupe.
  for( pair<string, value>& kv_source :
       ( **source_tbl ).members_ )
    ( **target_tbl )
        .members_.push_back( std::move( kv_source ) );

  UNWRAP_RETURN( deduped, std::move( **target_tbl ).dedupe() );
  v_target = make_unique<table>( std::move( deduped ) );
  return valid;
}

expect<list, string> list::dedupe_tables() && {
  list l;
  for( value& v : members_ ) {
    UNWRAP_RETURN( deduped, std::visit( dedupe_visitor{},
                                        std::move( v ) ) );
    l.members_.push_back( std::move( deduped ) );
  }
  return l;
}

expect<table, string> table::dedupe() && {
  vector<string>               order;
  unordered_map<string, value> m;
  for( auto& [k, v] : members_ ) {
    // First recursively dedupe the value.
    UNWRAP_RETURN( deduped_v, std::visit( dedupe_visitor{},
                                          std::move( v ) ) );
    // !! Do not use kv.v from here on!
    if( !m.contains( k ) ) {
      order.push_back( k );
      m.emplace( k, std::move( deduped_v ) );
      continue;
    }
    HAS_VALUE_OR_RET(
        merge_values( k, m[k], std::move( deduped_v ) ) );
  }
  DCHECK( order.size() == m.size() );
  table t;
  for( string const& k : order )
    // !! Don't move k here since we need it in both expressions.
    t.members_.push_back( { k, std::move( m[k] ) } );
  return t;
}

/****************************************************************
** Table Key Mapping
*****************************************************************/
namespace {

struct mapping_visitor {
  void operator()( null_t ) const {}
  void operator()( bool ) const {}
  void operator()( int ) const {}
  void operator()( double ) const {}
  void operator()( string& ) const {}

  void operator()( unique_ptr<table>& o ) const {
    DCHECK( o != nullptr );
    o->map_members();
  }

  void operator()( unique_ptr<list>& o ) const {
    DCHECK( o != nullptr );
    o->map_members();
  }
};

} // namespace

void table::map_members() & {
  for( auto& [key, val] : members_ ) {
    map_[key] = &val;
    std::visit( mapping_visitor{}, val );
  }
}

void list::map_members() & {
  for( value& val : members_ )
    std::visit( mapping_visitor{}, val );
}

/****************************************************************
** Post-processing Routine
*****************************************************************/
base::expect<table, string> run_postprocessing( table&& v1 ) {
  table v2 = std::move( v1 ).key_parser();
  table v3 = std::move( v2 ).unflatten();
  // Dedupe must happen after unflattening.
  UNWRAP_RETURN( v4, std::move( v3 ).dedupe() );
  // Mapping should be last.
  v4.map_members();
  return std::move( v4 );
}

base::expect<list, string> run_postprocessing( list&& v1 ) {
  list v2 = std::move( v1 ).key_parser();
  list v3 = std::move( v2 ).unflatten();
  // Dedupe must happen after unflattening.
  UNWRAP_RETURN( v4, std::move( v3 ).dedupe_tables() );
  // Mapping should be last.
  v4.map_members();
  return std::move( v4 );
}

/****************************************************************
** Document
*****************************************************************/
expect<doc> doc::create( table&& tbl ) {
  UNWRAP_RETURN( postprocessed,
                 run_postprocessing( std::move( tbl ) ) );
  return doc( std::move( postprocessed ) );
}

} // namespace rcl
