/****************************************************************
**recursive.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-05-23.
*
* Description: Helpers for doing recursive traversal.
*
*****************************************************************/
#include "recursive.hpp"

namespace trv {

namespace {

using namespace std;

} // namespace

/****************************************************************
** RecursiveTraverserWithTracking
*****************************************************************/
std::string RecursiveTraverserWithTracking::path() const {
  std::string p;
  p.reserve( len_ );
  for( auto const& e : keys_ ) p.append( e );
  CHECK_EQ( len_, ssize( p ) );
  return p;
}

void RecursiveTraverserWithTracking::push_key(
    std::string&& key ) {
  // If this is the first non-empty key in the path and if it
  // starts with a . then remove it.
  if( len_ == 0 && !key.empty() && key[0] == '.' )
    key.erase( 0, 1 );
  len_ += key.size();
  keys_.push_back( std::move( key ) );
}

void RecursiveTraverserWithTracking::pop_key() {
  CHECK( !keys_.empty() );
  len_ -= keys_.back().size();
  CHECK_GE( len_, 0 );
  keys_.pop_back();
}

} // namespace trv
