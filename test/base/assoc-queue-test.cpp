/****************************************************************
**assoc-queue-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-10-18.
*
* Description: Unit tests for the base/assoc-queue module.
*
*****************************************************************/
// Under test.
#include "src/base/assoc-queue.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace base {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[base/assoc-queue] construction" ) {
  AssociativeQueue<int> q;

  REQUIRE( q.empty() );
  REQUIRE( q.size() == 0 );
}

TEST_CASE( "[base/assoc-queue] push/pop/front" ) {
  AssociativeQueue<int> q;

  REQUIRE( q.empty() );
  REQUIRE( q.size() == 0 );

  q.push( 5 );
  REQUIRE_FALSE( q.empty() );
  REQUIRE( q.size() == 1 );
  REQUIRE( q.front() == 5 );

  q.push( 7 );
  REQUIRE_FALSE( q.empty() );
  REQUIRE( q.size() == 2 );
  REQUIRE( q.front() == 5 );

  q.pop();
  REQUIRE_FALSE( q.empty() );
  REQUIRE( q.size() == 1 );
  REQUIRE( q.front() == 7 );

  q.push( 9 );
  REQUIRE_FALSE( q.empty() );
  REQUIRE( q.size() == 2 );
  REQUIRE( q.front() == 7 );

  q.pop();
  REQUIRE_FALSE( q.empty() );
  REQUIRE( q.size() == 1 );
  REQUIRE( q.front() == 9 );

  q.pop();
  REQUIRE( q.empty() );
  REQUIRE( q.size() == 0 );
}

TEST_CASE( "[base/assoc-queue] erase" ) {
  AssociativeQueue<int> q;

  REQUIRE( q.empty() );
  REQUIRE( q.size() == 0 );

  q.push( 5 );
  REQUIRE_FALSE( q.empty() );
  REQUIRE( q.size() == 1 );
  REQUIRE( q.front() == 5 );

  q.push( 7 );
  REQUIRE_FALSE( q.empty() );
  REQUIRE( q.size() == 2 );
  REQUIRE( q.front() == 5 );

  q.erase( 9 );
  REQUIRE_FALSE( q.empty() );
  REQUIRE( q.size() == 2 );
  REQUIRE( q.front() == 5 );

  q.erase( 5 );
  REQUIRE_FALSE( q.empty() );
  REQUIRE( q.size() == 1 );
  REQUIRE( q.front() == 7 );

  q.erase( 7 );
  REQUIRE( q.empty() );
  REQUIRE( q.size() == 0 );
}

} // namespace
} // namespace base
