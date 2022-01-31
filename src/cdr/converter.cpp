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

// Abseil
#include "absl/strings/str_replace.h"

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

void converter::start_field_tracking() { used_keys_.clear(); }

base::valid_or<error> converter::end_field_tracking(
    table const& tbl ) {
  if( options_.allow_unrecognized_fields ) return base::valid;
  for( auto const& [k, v] : tbl )
    if( !used_keys_.contains( k ) )
      return err( "unrecognized key '{}' in table.", k );
  return base::valid;
}

vector<string> const& converter::error_stack() const {
  return frames_on_error_;
}

string converter::dump_error_stack() const {
  string out;
  out += "frame trace (most recent frame last):\n";
  out += "---------------------------------------------------\n";
  string spaces;
  // This will probably need to be tweaked when compiler versions
  // change. The idea is to make whatever substitutions are nec-
  // essary to make the output clean and readable.
  static vector<pair<string, string>> to_replace{
      { base::demangled_typename<string>(), "std::string" },
      { "::__1", "" },
      { " >", ">" },
  };
  for( string const& frame : error_stack() ) {
    string sanitized = absl::StrReplaceAll( frame, to_replace );
    if( sanitized.size() > 62 ) {
      sanitized.resize( 62 );
      sanitized = base::trim( sanitized );
      sanitized += "...";
    }
    out += fmt::format( "{}{}\n", spaces, sanitized );
    if( spaces.empty() )
      spaces = " \\-";
    else
      spaces = "   " + spaces;
  }
  out += "---------------------------------------------------";
  return out;
}

error converter::from_canonical_readable_error(
    error const& err_obj ) const {
  return error( fmt::format( "message: {}\n", err_obj.what() ) +
                dump_error_stack() );
}

} // namespace cdr
