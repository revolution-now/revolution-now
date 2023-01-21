/****************************************************************
**irand.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-19.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#include "irand.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Helpers
*****************************************************************/
// This will take a vector of indices representing how we'd like
// a vector shuffled and will generate all of the mock expect
// calls to make that happen.
void expect_shuffle( MockIRand&         rand,
                     vector<int> const& indices ) {
  if( indices.empty() ) return;
  int const last_elem = int( indices.size() ) - 1;
  // start: 0 1 2 3
  // end:   3 2 0 1
  //
  // i                    idx   slot   indices    simulation
  //      0 1 2 3                      3 2 0 1    0 1 2 3
  //
  // 0    swap( 0, 3 )      3      3
  //      3 1 2 0                      3 2 0 1    3 1 2 0
  //
  // 1    swap( 1, 2 )      2      2
  //      3 2 1 0                      3 2 0 1    3 2 1 0
  //
  // 2    swap( 2, 3 )      0      3
  //      3 2 0 1                      3 2 0 1    3 2 0 1
  //
  vector<int> simulation;
  simulation.reserve( indices.size() );
  for( int i = 0; i < int( indices.size() ); ++i )
    simulation.push_back( i );
  for( int i = 0; i < last_elem; ++i ) {
    int const idx = indices[i];
    auto it = find( simulation.begin(), simulation.end(), idx );
    CHECK( it != simulation.end() );
    int const slot = it - simulation.begin();
    CHECK_GE( slot, 0 );
    EXPECT_CALL(
        rand, between_ints( i, last_elem, e_interval::closed ) )
        .returns( slot );
    using std::swap;
    swap( simulation[i], simulation[slot] );
  }
}

} // namespace rn
