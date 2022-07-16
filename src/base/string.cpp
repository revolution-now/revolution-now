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

string str_replace( string_view sv, string_view from,
                    string_view to ) {
  string res;
  if( from.size() == 0 ) return string( sv );
  size_t pos   = 0;
  size_t where = sv.find( from, pos );
  res.reserve( sv.size() );
  while( where != string_view::npos ) {
    while( pos < where ) res.push_back( sv[pos++] );
    res += to;
    pos += from.size();
    where = sv.find( from, pos );
  }
  while( pos < sv.size() ) res.push_back( sv[pos++] );
  return res;
}

string str_replace_all(
    string_view                            sv,
    initializer_list<pair<string, string>> pairs ) {
  string res( sv );
  for( auto const& [from, to] : pairs )
    res = str_replace( res, from, to );
  return res;
}

} // namespace base
