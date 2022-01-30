/****************************************************************
**error.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-29.
*
* Description: Types for error handling in Cdr conversion.
*
*****************************************************************/
#include "error.hpp"

using namespace std;

namespace cdr {

bool error::operator==( error const& rhs ) const {
  return what == rhs.what;
}

void to_str( error const& o, string& out, base::ADL_t ) {
  out += fmt::format( "Message: {}\n", o.what );
  out += "Frame Trace (most recent frame last):\n";
  out += "------------------------------------\n";
  string spaces;
  for( int i = o.frames.size() - 1; i >= 0; --i ) {
    auto& frame = o.frames[i];
    out += fmt::format( "{}{}\n", spaces, frame.type_name );
    if( spaces.empty() )
      spaces = " \\-";
    else
      spaces = "   " + spaces;
  }
  out += "------------------------------------\n";
}

} // namespace cdr
