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

// Abseil
#include "absl/strings/str_replace.h"

using namespace std;

namespace cdr {

vector<string> const& converter::error_stack() const {
  return frames_on_error_;
}

string dump_error_stack( vector<string> const& frames ) {
  string out;
  out += "frame trace (most recent frame last):\n";
  out += "---------------------------------------------------\n";
  string        spaces;
  static string demangled_string_name =
      base::demangled_typename<string>();
  // This will probably need to be tweaked when compiler versions
  // change. The idea is to make whatever substitutions are nec-
  // essary to make the output clean and readable.
  static vector<pair<string, string>> to_replace{
      { demangled_string_name, "std::string" },
      { "::__1", "" },
  };
  for( string const& frame : frames ) {
    string sanitized = absl::StrReplaceAll( frame, to_replace );
    out += fmt::format( "{}{}\n", spaces, sanitized );
    if( spaces.empty() )
      spaces = " \\-";
    else
      spaces = "   " + spaces;
  }
  out += "---------------------------------------------------";
  return out;
}

} // namespace cdr
