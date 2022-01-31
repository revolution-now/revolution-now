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

vector<string> const& converter::error_stack() const {
  return frames_on_error_;
}

string dump_error_stack( vector<string> const& frames ) {
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
  for( string const& frame : frames ) {
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

} // namespace cdr
