/****************************************************************
**converter.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-31.
*
* Description: Helper for calling {to,from}_canonical.
*
*****************************************************************/
#include "converter.hpp"

// base
#include "base/string.hpp"

using namespace std;

namespace cdr {

base::valid_or<error> converter::ensure_list_size(
    list const& lst, int expected_size ) {
  if( lst.ssize() != expected_size )
    return err(
        "expected list of size {}, instead found size {}.",
        expected_size, lst.size() );
  return base::valid;
}

base::valid_or<error> converter::ensure_table_size(
    table const& tbl, int expected_size ) {
  if( tbl.ssize() != expected_size )
    return err(
        "expected table with {} key(s), instead found {} "
        "key(s).",
        expected_size, tbl.size() );
  return base::valid;
}

base::valid_or<error> converter::end_field_tracking(
    table const& tbl, set<string> const& used_keys ) {
  if( options_.allow_unrecognized_fields ) return base::valid;
  for( auto const& [k, v] : tbl )
    if( !used_keys.contains( k ) )
      return err( "unrecognized key '{}' in table.", k );
  return base::valid;
}

vector<string> const& converter::error_stack() const {
  return frames_on_error_;
}

string converter::dump_error_stack() const {
  string out;
  if( error_stack().empty() ) return out;
  out += "frame trace (most recent frame last):\n";
  string spaces;
  for( string const& frame : error_stack() ) {
    out += fmt::format( "{}{}\n", spaces, frame );
    if( spaces.empty() )
      spaces = " \\-";
    else
      spaces = " " + spaces;
  }
  if( !out.empty() )
    out.resize( out.size() - 1 ); // cut traing newline.
  return out;
}

error converter::from_canonical_readable_error(
    error const& err_obj ) const {
  string const stack = dump_error_stack();
  if( stack.empty() )
    return error( err_obj.what() );
  else
    return error( format( "{}\n{}", err_obj.what(), stack ) );
}

} // namespace cdr
