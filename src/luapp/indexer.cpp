/****************************************************************
**indexer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-10.
*
* Description: Proxy for composing chains of index operators.
*
*****************************************************************/
#include "indexer.hpp"

// luapp
#include "c-api.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace lua {

namespace internal {

void indexer_gettable( cthread L ) {
  c_api C( L );
  CHECK( C.stack_size() >= 2 );
  C.gettable( -2 );
  C.swap_top();
  C.pop();
}

void indexer_settable( cthread L ) {
  c_api C( L );
  CHECK( C.stack_size() >= 3 );
  C.settable( -3 );
  C.pop();
}

bool indexer_eq( cthread L ) {
  c_api C( L );
  CHECK( C.stack_size() >= 2 );
  bool res = C.compare_eq( -2, -1 );
  C.pop( 2 );
  return res;
}

void indexer_pop( cthread L, int n ) { c_api( L ).pop( n ); }

} // namespace internal

} // namespace lua
