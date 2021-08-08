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
  return members_[n].v;
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
    return value{ make_unique<table>(
        table::unflatten( std::move( *o ) ) ) };
  }

  value operator()( unique_ptr<list>&& o ) const {
    return value{ make_unique<list>(
        list::unflatten( std::move( *o ) ) ) };
  }
};

} // namespace

void table::unflatten_impl( string_view dotted, value&& v ) {
  int i = dotted.find_first_of( "." );
  if( i == int( string_view::npos ) ) {
    members_.push_back( key_val{
        string( dotted ),
        std::visit( unflatten_visitor{}, std::move( v ) ) } );
    return;
  }
  DCHECK( i + 1 < int( dotted.size() ) );
  string key  = string( dotted.substr( 0, i ) );
  string rest = string( dotted.substr( i + 1 ) );
  // Add a new one even if there is already a key with the same
  // name in the table, because our tables allow duplicate keys.
  key_val& new_kv = members_.emplace_back();
  new_kv.k        = key;
  new_kv.v        = value{ make_unique<table>() };
  table* tbl      = new_kv.v.get<unique_ptr<table>>().get();
  tbl->unflatten_impl( rest, std::move( v ) );
}

table table::unflatten( table&& old ) {
  table t;
  for( key_val& kv : old.members_ )
    t.unflatten_impl( kv.k, std::move( kv.v ) );
  return t;
}

list list::unflatten( list&& old ) {
  list l;
  for( value& v : old.members_ )
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
    UNWRAP_RETURN( deduped, table::dedupe( std::move( *o ) ) );
    return value{ make_unique<table>( std::move( deduped ) ) };
  }

  expect<value, string> operator()(
      unique_ptr<list>&& o ) const {
    UNWRAP_RETURN( deduped,
                   list::dedupe_tables( std::move( *o ) ) );
    return value{ make_unique<list>( std::move( deduped ) ) };
  }
};

} // namespace

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
  for( table::key_val& kv_source : ( **source_tbl ).members_ )
    ( **target_tbl )
        .members_.push_back( std::move( kv_source ) );

  UNWRAP_RETURN( deduped,
                 table::dedupe( std::move( **target_tbl ) ) );
  v_target = make_unique<table>( std::move( deduped ) );
  return valid;
}

expect<list, string> list::dedupe_tables( list&& old ) {
  list l;
  for( value& v : old.members_ ) {
    UNWRAP_RETURN( deduped, std::visit( dedupe_visitor{},
                                        std::move( v ) ) );
    l.members_.push_back( std::move( deduped ) );
  }
  return l;
}

expect<table, string> table::dedupe( table&& old ) {
  vector<string>               order;
  unordered_map<string, value> m;
  for( table::key_val& kv : old.members_ ) {
    // First recursively dedupe the value.
    UNWRAP_RETURN( deduped_v, std::visit( dedupe_visitor{},
                                          std::move( kv.v ) ) );
    // !! Do not use kv.v from here on!
    if( !m.contains( kv.k ) ) {
      order.push_back( kv.k );
      m.emplace( kv.k, std::move( deduped_v ) );
      continue;
    }
    HAS_VALUE_OR_RET(
        merge_values( kv.k, m[kv.k], std::move( deduped_v ) ) );
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
void map_members( table* tbl ) {
  using T = unique_ptr<table>;
  for( auto& [key, val] : tbl->members_ ) {
    tbl->map_[key] = &val;
    if( maybe<T&> table_val = val.get_if<T>(); table_val )
      map_members( table_val->get() );
  }
}

/****************************************************************
** Document Postprocessing
*****************************************************************/
expect<doc, std::string> doc::create( table&& v1 ) {
  table v2 = table::unflatten( std::move( v1 ) );
  // Dedupe must happen after unflattening.
  UNWRAP_RETURN( v3, table::dedupe( std::move( v2 ) ) );
  // Mapping should be last.
  map_members( &v3 );

  return doc( std::move( v3 ) );
}

} // namespace rcl
