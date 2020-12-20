/****************************************************************
**moving-avg.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-19.
*
* Description: Unit tests for the src/moving-avg.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/moving-avg.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::Catch::Detail::Approx;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;
using std::chrono::seconds;

chrono::system_clock::time_point g_now{};

auto Now() { return g_now; }
void AdvanceClock( milliseconds d ) { g_now += d; }

// Unfortunately this test case depends on the number of buckets
// used by the MovingAverage. If we don't know that then it is
// probably difficult to compare averages because they will be
// tricky to predict exactly.
TEST_CASE( "[moving-avg] correctness" ) {
  MovingAverage ma( seconds( 1 ), Now );

  REQUIRE( ma.average() == 0 );

  // Create a stable average of 250.0/sec and verify that the av-
  // erage rises to that level.
  for( int i = 0; i < 25; ++i ) {
    AdvanceClock( milliseconds( 40 ) );
    ma.tick( 10 );
    REQUIRE( ma.average() == Approx( double( i ) * 10.0 ) );
  }

  // Maintain the same average.
  for( int i = 0; i < 25; ++i ) {
    AdvanceClock( milliseconds( 40 ) );
    ma.tick( 10 );
    REQUIRE( ma.average() == Approx( 250.0 ) );
  }

  // Slow it down to 125.0/sec.
  for( int i = 0; i < 25; ++i ) {
    AdvanceClock( milliseconds( 40 ) );
    ma.tick( 5 );
    REQUIRE( ma.average() ==
             Approx( 250.0 * 0.5 * ( 1 + ( 25.0 - i ) / 25 ) ) );
  }

  // Maintain the same average.
  for( int i = 0; i < 25; ++i ) {
    AdvanceClock( milliseconds( 40 ) );
    ma.tick( 5 );
    REQUIRE( ma.average() == Approx( 125.0 ) );
  }

  // Slow it down to zero.
  for( int i = 0; i < 25; ++i ) {
    AdvanceClock( milliseconds( 40 ) );
    ma.update();
    REQUIRE( ma.average() ==
             Approx( 125.0 * ( 25.0 - i ) / 25.0 ) );
  }

  REQUIRE( ma.average() == Approx( 5.0 ) );

  // One more should bring it to zero.
  AdvanceClock( milliseconds( 40 ) );
  ma.update();
  REQUIRE( ma.average() == Approx( 0 ) );
}

} // namespace
} // namespace rn
