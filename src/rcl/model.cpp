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

struct value_printer {
  string_view indent;

  string operator()( bool b ) { return b ? "true" : "false"; }

  string operator()( int n ) { return fmt::to_string( n ); }

  string operator()( double d ) { return fmt::to_string( d ); }

  string operator()( string const& s ) {
    return fmt::format( "\"{}\"", s );
  }

  string operator()( std::unique_ptr<table> const& tbl ) {
    return tbl->pretty_print( indent );
  }

  string operator()( std::unique_ptr<list> const& lst ) {
    return lst->pretty_print( indent );
  }
};

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

value const& table::operator[]( int n ) const {
  CHECK( n >= 0 && n < int( members_.size() ),
         "invalid table index {}", n );
  return members_[n].second;
}

string table::pretty_print( string_view indent ) const {
  bool          is_top_level = ( indent.size() == 0 );
  ostringstream oss;
  if( indent.size() > 0 ) oss << fmt::format( "{{\n" );

  size_t n = members_.size();
  for( auto& [k, v] : members_ ) {
    string assign = ":";
    if( v.holds<unique_ptr<table>>() ) assign = "";
    oss << fmt::format(
        "{}{}{} {}\n", indent, k, assign,
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
  return tbl_.pretty_print( indent );
}

/****************************************************************
** Table Flattening
*****************************************************************/
namespace {

struct unflatten_visitor {
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
  int i = dotted.find_first_of( "." );
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
** Document Postprocessing
*****************************************************************/
expect<doc, std::string> doc::create( table&& v1 ) {
  table v2 = std::move( v1 ).unflatten();
  // Dedupe must happen after unflattening.
  UNWRAP_RETURN( v3, std::move( v2 ).dedupe() );
  // Mapping should be last.
  v3.map_members();

  return doc( std::move( v3 ) );
}

} // namespace rcl
