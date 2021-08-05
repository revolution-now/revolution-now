/****************************************************************
**model.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Document model for cl (config language) files.
*
*****************************************************************/
#include "model.hpp"

// base
#include "base/valid.hpp" // FIXME: remove

// C++ standard library
#include <sstream>
#include <unordered_map>

using namespace std;

namespace cl {

using ::base::expect;
using ::base::maybe;
using ::base::nothing;
using ::base::valid;
using ::base::valid_or;

struct value_printer {
  string_view indent;

  string operator()( boolean b ) {
    return b.b ? "true" : "false";
  }

  string operator()( number n ) {
    return fmt::to_string( n.val );
  }

  string operator()( string_val const& s ) {
    return fmt::format( "\"{}\"", s.val );
  }

  string operator()( std::unique_ptr<table> const& tbl ) {
    return tbl->pretty_print( indent );
  }

  string operator()( std::unique_ptr<list> const& lst ) {
    return lst->pretty_print( indent );
  }
};

/****************************************************************
**table
*****************************************************************/
maybe<value&> table::operator[]( string_view key ) {
  for( key_val& kv : members )
    if( kv.k == key ) return kv.v;
  return nothing;
}

maybe<value const&> table::operator[]( string_view key ) const {
  for( key_val const& kv : members )
    if( kv.k == key ) return kv.v;
  return nothing;
}

string table::pretty_print( string_view indent ) const {
  bool          is_top_level = ( indent.size() == 0 );
  ostringstream oss;
  if( indent.size() > 0 ) oss << fmt::format( "{{\n" );

  size_t n = members.size();
  for( auto& [k, v] : members ) {
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

string list::pretty_print( string_view indent ) const {
  ostringstream oss;
  if( indent.size() > 0 ) oss << fmt::format( "[\n" );

  for( auto& v : members )
    oss << fmt::format(
        "{}{},\n", indent,
        std::visit( value_printer{ string( indent ) + "  " },
                    v ) );

  indent.remove_suffix( 2 );
  oss << fmt::format( "{}]", indent );

  return oss.str();
}

namespace {

// Consumes the old table, which will be left empty (mostly).
table unflatten_table( table&& old );
list  unflatten_list( list&& old );

struct unflatten_visitor {
  value operator()( boolean&& o ) const {
    return value{ std::move( o ) };
  }
  value operator()( number&& o ) const {
    return value{ std::move( o ) };
  }
  value operator()( string_val&& o ) const {
    return value{ std::move( o ) };
  }
  value operator()( unique_ptr<table>&& o ) const {
    return value{ make_unique<table>(
        unflatten_table( std::move( *o ) ) ) };
  }
  value operator()( unique_ptr<list>&& o ) const {
    return value{
        make_unique<list>( unflatten_list( std::move( *o ) ) ) };
  }
};

void unflatten_table_impl( table* tbl, string_view dotted,
                           value&& v ) {
  int i = dotted.find_first_of( "." );
  if( i == int( string_view::npos ) ) {
    tbl->members.push_back( key_val(
        string( dotted ),
        std::visit( unflatten_visitor{}, std::move( v ) ) ) );
    return;
  }
  DCHECK( i + 1 < int( dotted.size() ) );
  string key  = string( dotted.substr( 0, i ) );
  string rest = string( dotted.substr( i + 1 ) );
  // Add a new one even if there is already a key with the same
  // name in the table, because our tables allow duplicate keys.
  key_val& new_kv = tbl->members.emplace_back();
  new_kv.k        = key;
  new_kv.v        = value{ make_unique<table>() };
  tbl             = new_kv.v.get<unique_ptr<table>>().get();
  unflatten_table_impl( tbl, rest, std::move( v ) );
}

list unflatten_list( list&& old ) {
  list l;
  for( value& v : old.members )
    l.members.push_back(
        std::visit( unflatten_visitor{}, std::move( v ) ) );
  return l;
}

table unflatten_table( table&& old ) {
  table t;
  for( key_val& kv : old.members )
    unflatten_table_impl( &t, kv.k, std::move( kv.v ) );
  return t;
}

base::expect<table, std::string> dedupe_table( table&& old );
base::expect<list, std::string>  dedupe_tables_in_list(
     list&& old );

struct dedupe_visitor {
  expect<value, string> operator()( boolean&& o ) const {
    return value{ std::move( o ) };
  }
  expect<value, string> operator()( number&& o ) const {
    return value{ std::move( o ) };
  }
  expect<value, string> operator()( string_val&& o ) const {
    return value{ std::move( o ) };
  }
  expect<value, string> operator()(
      unique_ptr<table>&& o ) const {
    UNWRAP_RETURN( deduped, dedupe_table( std::move( *o ) ) );
    return value{ make_unique<table>( std::move( deduped ) ) };
  }
  expect<value, string> operator()(
      unique_ptr<list>&& o ) const {
    UNWRAP_RETURN( deduped,
                   dedupe_tables_in_list( std::move( *o ) ) );
    return value{ make_unique<list>( std::move( deduped ) ) };
  }
};

expect<list, string> dedupe_tables_in_list( list&& old ) {
  list l;
  for( value& v : old.members ) {
    UNWRAP_RETURN( deduped, std::visit( dedupe_visitor{},
                                        std::move( v ) ) );
    l.members.push_back( std::move( deduped ) );
  }
  return l;
}

// Note: target is an lvalue ref and the source is an rvalue ref.
valid_or<string> merge_values( string_view key, value& v_target,
                               value&& v_source ) {
  // The two values can only be merged if they are both tables,
  // so make sure that they are and fail otherwise.
  maybe<unique_ptr<table>&> target_tbl =
      v_target.get_if<unique_ptr<table>>();
  maybe<unique_ptr<table>&> source_tbl =
      v_source.get_if<unique_ptr<table>>();
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
  for( key_val& kv_source : ( **source_tbl ).members )
    ( **target_tbl ).members.push_back( std::move( kv_source ) );

  UNWRAP_RETURN( deduped,
                 dedupe_table( std::move( **target_tbl ) ) );
  v_target = make_unique<table>( std::move( deduped ) );
  return valid;
}

expect<table, string> dedupe_table( table&& old ) {
  vector<string>               order;
  unordered_map<string, value> m;
  for( key_val& kv : old.members ) {
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
    t.members.emplace_back( k, std::move( m[k] ) );
  return t;
}

expect<table, string> post_process_table( table&& old ) {
  // Dedupe must happen after unflattening.
  table flattened = unflatten_table( std::move( old ) );
  UNWRAP_RETURN( deduped,
                 dedupe_table( std::move( flattened ) ) );
  return std::move( deduped );
}

} // namespace

base::expect<doc, std::string> doc::create( rawdoc rdoc ) {
  UNWRAP_RETURN( final_tbl,
                 post_process_table( std::move( rdoc.tbl ) ) );
  return doc( std::move( final_tbl ) );
}

} // namespace cl
