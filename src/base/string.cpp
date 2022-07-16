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

// base
#include "error.hpp"

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

vector<string> str_split( string_view sv, char c ) {
  return str_split_on_any( sv, string_view( &c, 1 ) );
}

vector<string> str_split_on_any( string_view sv,
                                 string_view chars ) {
  vector<string> res;
  while( true ) {
    auto next = sv.find_first_of( chars );
    if( next == string_view::npos ) break;
    res.push_back( string( sv.substr( 0, next ) ) );
    // Remove what we just added, plus c.
    sv.remove_prefix( next + 1 );
  }
  res.push_back( string( sv ) );
  return res;
}

string str_join( vector<string> const& v,
                 string_view const     sep ) {
  if( !v.size() ) return "";
  // First attempt to compute how much space we need, which  we
  // should be able to do exactly.
  size_t total = 0;
  for( auto const& e : v ) total += e.size();
  total += sep.size() * ( v.size() - 1 ); // v.size() > 0 always
  // Now construct the result (reserve +1 for good measure).
  std::string res;
  res.reserve( total + 1 );
  bool first = true;
  for( auto const& e : v ) {
    if( !first ) res += sep;
    res += e;
    first = false;
  }
  // Just to make sure  we  did  the  calculation right; if not,
  // then we might pay extra in memory allocations.
  DCHECK( res.size() == total,
          "res.size() == {} and total == {}", res.size(),
          total );
  return res;
}

bool str_contains( string_view haystack, string_view needle ) {
  return haystack.find( needle ) != string_view::npos;
}

string ascii_str_to_lower( string_view sv ) {
  string res;
  res.reserve( sv.size() );
  for( char c : sv ) res.push_back( std::tolower( c ) );
  return res;
}

} // namespace base
