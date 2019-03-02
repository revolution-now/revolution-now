/****************************************************************
**cache.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-03-01.
*
* Description: Utilities for caching and memoization.
*
*****************************************************************/
#include "cache.hpp"

// Revolution Now
#include "logging.hpp"

namespace rn {

namespace {

int fake_frames = 1;

int calc0() { return fake_frames * 3; }
int calc( int x ) { return x + 1; }

} // namespace

/****************************************************************
** Just for testing
*****************************************************************/
void test_memoizer() {
  auto memoized_calc = memoizer( calc, [] { return false; } );

  logger->debug( "memoized_calc(1): {}", memoized_calc( 1 ) );
  logger->debug( "memoized_calc(1): {}", memoized_calc( 1 ) );
  logger->debug( "memoized_calc(1): {}", memoized_calc( 1 ) );
  logger->debug( "memoized_calc(2): {}", memoized_calc( 2 ) );
  logger->debug( "memoized_calc(2): {}", memoized_calc( 2 ) );
  logger->debug( "memoized_calc(2): {}", memoized_calc( 2 ) );
  logger->debug( "memoized_calc(3): {}", memoized_calc( 3 ) );
  logger->debug( "memoized_calc(3): {}", memoized_calc( 3 ) );
  logger->debug( "memoized_calc(3): {}", memoized_calc( 3 ) );

  auto invalidator = [&] {
    static auto current = fake_frames;
    if( current != fake_frames ) {
      current = fake_frames;
      return true;
    }
    return false;
  };

  auto memoized_calc0 =
      memoizer( calc0, std::move( invalidator ) );

  logger->debug( "memoized_calc(): {}", memoized_calc0() );
  logger->debug( "memoized_calc(): {}", memoized_calc0() );
  logger->debug( "memoized_calc(): {}", memoized_calc0() );
  logger->debug( "memoized_calc(): {}", memoized_calc0() );
  fake_frames++;
  logger->debug( "memoized_calc(): {}", memoized_calc0() );
  logger->debug( "memoized_calc(): {}", memoized_calc0() );
  logger->debug( "memoized_calc(): {}", memoized_calc0() );
  fake_frames++;
  logger->debug( "memoized_calc(): {}", memoized_calc0() );
  logger->debug( "memoized_calc(): {}", memoized_calc0() );
}

} // namespace rn
