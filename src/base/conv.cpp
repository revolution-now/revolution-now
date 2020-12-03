/****************************************************************
**conv.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-02.
*
* Description: Utilities for conversions between numbers and
*              strings.
*
*****************************************************************/
#include "conv.hpp"

// C++ standard library
#include <string>

using namespace std;

namespace base {

/****************************************************************
 * From-String utilities
 ****************************************************************/
maybe<int> stoi( string const& s, int base ) {
  maybe<int> res;
  if( !s.empty() ) {
    size_t written;
    try {
      auto n = ::std::stoi( s, &written, base );
      if( written == s.size() ) res = n;
    } catch( std::exception const& ) {}
  }
  return res;
}

} // namespace base
