/****************************************************************
**ranges-lite.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-16.
*
* Description: Unit tests for the src/base/ranges-lite.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/base/ranges-lite.hpp"

// base
#include "base/lambda.hpp"

// Must be last.
#include "catch-common.hpp"

namespace base {
namespace {

using namespace std;

using ::Catch::Equals;

TEST_CASE( "[ranges-lite] pipeline" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = rl::view( input )              //
                 .keep( L( _ % 2 == 1 ) )   // 1,3,5,7,9
                 .map( L( _ * _ ) )         // 1,9,25,49,81
                 .map( L( _ + 1 ) )         // 2,10,26,50,82
                 .take_while( L( _ < 27 ) ) // 2,10,26
                 .materialize();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 10, 26 } ) );
}

TEST_CASE( "[ranges-lite] rview" ) {
  vector<int> input{ 9, 8, 7, 6, 5, 4, 3, 2, 1 };

  auto vec = rl::rview( input )             //
                 .keep( L( _ % 2 == 1 ) )   // 1,3,5,7,9
                 .map( L( _ * _ ) )         // 1,9,25,49,81
                 .map( L( _ + 1 ) )         // 2,10,26,50,82
                 .take_while( L( _ < 27 ) ) // 2,10,26
                 .materialize();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 10, 26 } ) );
}

} // namespace
} // namespace base
