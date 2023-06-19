/****************************************************************
**to-str-ext-chrono.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-23.
*
* Description: Unit tests for the src/base/to-str-ext-chrono.*
*              module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/to-str-ext-chrono.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[base/to-str-ext-chrono] format_duration" ) {
  using namespace std::literals::chrono_literals;

  auto f = []( chrono::nanoseconds ns ) {
    return format_duration( ns );
  };

  // Nanoseconds.
  REQUIRE( f( 0ns ) == "0ns" );
  REQUIRE( f( 1ns ) == "1ns" );
  REQUIRE( f( 101ns ) == "101ns" );
  REQUIRE( f( 999ns ) == "999ns" );

  // Microseconds/nanoseconds.
  REQUIRE( f( 1'000ns ) == "1us" );
  REQUIRE( f( 1'010ns ) == "1us10ns" );
  REQUIRE( f( 1'010ns ) == "1us10ns" );
  REQUIRE( f( 9'010ns ) == "9us10ns" );

  // Microseconds.
  REQUIRE( f( 10'010ns ) == "10us" );
  REQUIRE( f( 101'010ns ) == "101us" );
  REQUIRE( f( 999'999ns ) == "999us" );

  // Millseconds/microseconds.
  REQUIRE( f( 1'000'000ns ) == "1ms" );
  REQUIRE( f( 1'010'000ns ) == "1ms10us" );
  REQUIRE( f( 1'010'000ns ) == "1ms10us" );
  REQUIRE( f( 9'010'000ns ) == "9ms10us" );

  // Milliseconds.
  REQUIRE( f( 10'010'000ns ) == "10ms" );
  REQUIRE( f( 101'010'000ns ) == "101ms" );
  REQUIRE( f( 999'999'000ns ) == "999ms" );

  // Seconds/milliseconds.
  REQUIRE( f( 1'000'000'000ns ) == "1s" );
  REQUIRE( f( 1'010'000'000ns ) == "1s10ms" );
  REQUIRE( f( 1'010'000'000ns ) == "1s10ms" );
  REQUIRE( f( 9'010'000'000ns ) == "9s10ms" );

  // Seconds.
  REQUIRE( f( 10'010'000'000ns ) == "10s" );
  REQUIRE( f( 51'010'000'000ns ) == "51s" );
  REQUIRE( f( 59'999'999'999ns ) == "59s" );

  // Minutes/seconds.
  REQUIRE( f( 60'000'000'000ns ) == "1m" );
  REQUIRE( f( 65'000'000'000ns ) == "1m5s" );
  REQUIRE( f( 120'000'000'000ns ) == "2m" );
  REQUIRE( f( 135'000'000'000ns ) == "2m15s" );
  REQUIRE( f( 720'000'000'000ns ) == "12m" );
  REQUIRE( f( 725'000'000'000ns ) == "12m5s" );
  REQUIRE( f( 1'199'000'000'000ns ) == "19m59s" );
  REQUIRE( f( 1'200'000'000'000ns ) == "20m" );

  // Minutes.
  REQUIRE( f( 3'599'000'000'000ns ) == "59m" );
  REQUIRE( f( 3'599'000'000'000ns ) == "59m" );

  // Hours/minutes.
  REQUIRE( f( 3'600'000'000'000ns ) == "1h" );
  REQUIRE( f( 3'600'000'000'000ns ) == "1h" );
  REQUIRE( f( 5'000'000'000'000ns ) == "1h23m" );
  REQUIRE( f( 71'999'000'000'000ns ) == "19h59m" );
  REQUIRE( f( 72'000'000'000'000ns ) == "20h" );

  // Hours.
  REQUIRE( f( 92'000'000'000'000ns ) == "25h" );
}

} // namespace
} // namespace base
