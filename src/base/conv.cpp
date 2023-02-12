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

maybe<double> stod( string const& s ) {
  maybe<double> res;
  if( !s.empty() ) {
    size_t written;
    try {
      auto d = ::std::stod( s, &written );
      if( written == s.size() ) res = d;
    } catch( std::exception const& ) {}
  }
  return res;
}

string int_to_string_literary( int n ) {
  switch( n ) {
    case 0:
      return "zero";
    case 1:
      return "one";
    case 2:
      return "two";
    case 3:
      return "three";
    case 4:
      return "four";
    case 5:
      return "five";
    case 6:
      return "six";
    case 7:
      return "seven";
    case 8:
      return "eight";
    case 9:
      return "nine";
    default:
      return fmt::to_string( n );
  }
}

} // namespace base
