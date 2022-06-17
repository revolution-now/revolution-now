/****************************************************************
**string.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-28.
*
* Description: String manipulation utilities.
*
*****************************************************************/
#include "string.hpp"

// C++ standard library
#include <string>
#include <string_view>

using namespace std;

namespace base {

string trim( string_view sv ) {
  auto start = sv.find_first_not_of( " \t\n\r" );
  sv.remove_prefix( min( start, sv.size() ) );
  // Must do this here because sv is being mutated.
  auto last = sv.find_last_not_of( " \t\n\r" ) + 1;
  sv.remove_suffix( sv.size() - min( last, sv.size() ) );
  return string( sv );
}

string capitalize_initials( string_view sv ) {
  string res;
  res.reserve( sv.size() );
  bool capitalize = true;
  for( char c : sv ) {
    res.push_back( capitalize ? toupper( c ) : c );
    capitalize = ( c == ' ' );
  }
  return res;
}

} // namespace base
