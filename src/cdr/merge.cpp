/****************************************************************
**merge.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-18.
*
* Description: Algorithm for merging two cdr tables.
*
*****************************************************************/
#include "merge.hpp"

using namespace std;

namespace cdr {

/****************************************************************
** Public API.
*****************************************************************/
void RightJoiner::operator()( value& l, value const& r ) {
  visit( *this, l, r );
}

void RightJoiner::operator()( table& l, table const& r ) {
  for( auto const& [k, v] : r ) {
    if( l.contains( k ) ) {
      ( *this )( l[k], v );
      continue;
    }
    l[k] = v;
    ++l_missing;
  }
}

void RightJoiner::operator()( list& l, list const& r ) {
  for( int i = 0; i < ssize( l ); ++i ) {
    if( i >= ssize( r ) ) continue;
    ( *this )( l[i], r[i] );
  }
  for( int i = ssize( l ); i < ssize( r ); ++i ) {
    l.push_back( r[i] );
    ++l_missing;
  }
}

void RightJoiner::operator()( auto&, auto const& ) const {
  return;
}

} // namespace cdr
