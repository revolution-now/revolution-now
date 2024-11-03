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
  REQUIRE( f( 1000ns ) == "1us" );
  REQUIRE( f( 1010ns ) == "1us10ns" );
  REQUIRE( f( 1010ns ) == "1us10ns" );
  REQUIRE( f( 9010ns ) == "9us10ns" );

  // Microseconds.
  REQUIRE( f( 10010ns ) == "10us" );
  REQUIRE( f( 101010ns ) == "101us" );
  REQUIRE( f( 999999ns ) == "999us" );

  // Millseconds/microseconds.
  REQUIRE( f( 1000000ns ) == "1ms" );
  REQUIRE( f( 1010000ns ) == "1ms10us" );
  REQUIRE( f( 1010000ns ) == "1ms10us" );
  REQUIRE( f( 9010000ns ) == "9ms10us" );

  // Milliseconds.
  REQUIRE( f( 10010000ns ) == "10ms" );
  REQUIRE( f( 101010000ns ) == "101ms" );
  REQUIRE( f( 999999000ns ) == "999ms" );

  // Seconds/milliseconds.
  REQUIRE( f( 1000000000ns ) == "1s" );
  REQUIRE( f( 1010000000ns ) == "1s10ms" );
  REQUIRE( f( 1010000000ns ) == "1s10ms" );
  REQUIRE( f( 9010000000ns ) == "9s10ms" );

  // Seconds.
  REQUIRE( f( 10010000000ns ) == "10s" );
  REQUIRE( f( 51010000000ns ) == "51s" );
  REQUIRE( f( 59999999999ns ) == "59s" );

  // Minutes/seconds.
  REQUIRE( f( 60000000000ns ) == "1m" );
  REQUIRE( f( 65000000000ns ) == "1m5s" );
  REQUIRE( f( 120000000000ns ) == "2m" );
  REQUIRE( f( 135000000000ns ) == "2m15s" );
  REQUIRE( f( 720000000000ns ) == "12m" );
  REQUIRE( f( 725000000000ns ) == "12m5s" );
  REQUIRE( f( 1199000000000ns ) == "19m59s" );
  REQUIRE( f( 1200000000000ns ) == "20m" );

  // Minutes.
  REQUIRE( f( 3599000000000ns ) == "59m" );
  REQUIRE( f( 3599000000000ns ) == "59m" );

  // Hours/minutes.
  REQUIRE( f( 3600000000000ns ) == "1h" );
  REQUIRE( f( 3600000000000ns ) == "1h" );
  REQUIRE( f( 5000000000000ns ) == "1h23m" );
  REQUIRE( f( 71999000000000ns ) == "19h59m" );
  REQUIRE( f( 72000000000000ns ) == "20h" );

  // Hours.
  REQUIRE( f( 92000000000000ns ) == "25h" );
}

} // namespace
} // namespace base
