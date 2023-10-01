/****************************************************************
**latch-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-30.
*
* Description: Unit tests for the latch module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/latch.hpp"

// Revolution Now
#include "src/co-scheduler.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn::co {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[co-combinator] latch" ) {
  SECTION( "counter=0" ) {
    latch l( 0 );
    REQUIRE( l.counter() == 0 );
    l.count_down( 0 ); // should be a no-op.
    run_all_cpp_coroutines();
    REQUIRE( l.counter() == 0 );
    REQUIRE( l.try_wait() );
    l.count_down( 0 ); // should be a no-op.
    run_all_cpp_coroutines();
    REQUIRE( l.Wait().ready() );
    REQUIRE( l.Wait().ready() );
    REQUIRE( l.Wait().ready() );
    REQUIRE( l.counter() == 0 );
  }

  SECTION( "counter=1, arrive_and_wait" ) {
    latch l;
    l.count_down( 0 );
    REQUIRE( l.counter() == 1 );
    run_all_cpp_coroutines();
    REQUIRE_FALSE( l.try_wait() );
    wait w1 = l.Wait();
    run_all_cpp_coroutines();
    REQUIRE_FALSE( w1.ready() );
    REQUIRE_FALSE( l.try_wait() );
    wait w2 = l.Wait();
    run_all_cpp_coroutines();
    REQUIRE_FALSE( w2.ready() );
    REQUIRE_FALSE( w1.ready() );
    REQUIRE_FALSE( l.try_wait() );
    wait w3 = l.Wait();
    run_all_cpp_coroutines();
    REQUIRE( l.counter() == 1 );
    REQUIRE_FALSE( w3.ready() );
    REQUIRE_FALSE( w2.ready() );
    REQUIRE_FALSE( w1.ready() );
    REQUIRE_FALSE( l.try_wait() );
    wait w4 = l.arrive_and_wait();
    REQUIRE( l.counter() == 0 );
    run_all_cpp_coroutines();
    REQUIRE( w1.ready() );
    REQUIRE( w2.ready() );
    REQUIRE( w3.ready() );
    REQUIRE( w4.ready() );
    REQUIRE( l.try_wait() );
    REQUIRE( l.counter() == 0 );
    REQUIRE( l.Wait().ready() );
    REQUIRE( l.Wait().ready() );
    REQUIRE( l.counter() == 0 );
  }

  SECTION( "counter=1, count_down" ) {
    latch l;
    REQUIRE( l.counter() == 1 );
    l.count_down( 0 );
    REQUIRE( l.counter() == 1 );
    REQUIRE_FALSE( l.try_wait() );
    wait w1 = l.Wait();
    run_all_cpp_coroutines();
    REQUIRE_FALSE( w1.ready() );
    REQUIRE_FALSE( l.try_wait() );
    wait w2 = l.Wait();
    run_all_cpp_coroutines();
    REQUIRE_FALSE( w2.ready() );
    REQUIRE_FALSE( w1.ready() );
    REQUIRE_FALSE( l.try_wait() );
    wait w3 = l.Wait();
    run_all_cpp_coroutines();
    REQUIRE_FALSE( w3.ready() );
    REQUIRE_FALSE( w2.ready() );
    REQUIRE_FALSE( w1.ready() );
    REQUIRE_FALSE( l.try_wait() );
    REQUIRE( l.counter() == 1 );
    l.count_down();
    REQUIRE( l.counter() == 0 );
    run_all_cpp_coroutines();
    REQUIRE( w1.ready() );
    REQUIRE( w2.ready() );
    REQUIRE( w3.ready() );
    REQUIRE( l.try_wait() );
    REQUIRE( l.Wait().ready() );
    REQUIRE( l.Wait().ready() );
    REQUIRE( l.counter() == 0 );
  }

  SECTION( "counter=3" ) {
    latch l( 3 );
    REQUIRE( l.counter() == 3 );
    wait<> w1 = l.arrive_and_wait();
    REQUIRE( l.counter() == 2 );
    run_all_cpp_coroutines();
    REQUIRE_FALSE( w1.ready() );
    REQUIRE_FALSE( l.try_wait() );
    wait<> w2 = l.arrive_and_wait();
    REQUIRE( l.counter() == 1 );
    run_all_cpp_coroutines();
    REQUIRE_FALSE( w2.ready() );
    REQUIRE_FALSE( w1.ready() );
    REQUIRE_FALSE( l.try_wait() );
    wait<> w3 = l.arrive_and_wait();
    REQUIRE( l.counter() == 0 );
    run_all_cpp_coroutines();
    REQUIRE( w3.ready() );
    REQUIRE( w2.ready() );
    REQUIRE( w1.ready() );
    REQUIRE( l.try_wait() );
    REQUIRE( l.Wait().ready() );
    l.count_down( 0 );
    REQUIRE( l.counter() == 0 );
    run_all_cpp_coroutines();
    REQUIRE( l.Wait().ready() );
    REQUIRE( l.counter() == 0 );
  }

  SECTION( "counter=10" ) {
    latch l( 10 );
    REQUIRE( l.counter() == 10 );
    run_all_cpp_coroutines();
    REQUIRE_FALSE( l.try_wait() );
    REQUIRE( l.counter() == 10 );
    wait<> w1 = l.arrive_and_wait( 5 );
    REQUIRE( l.counter() == 5 );
    run_all_cpp_coroutines();
    REQUIRE( l.counter() == 5 );
    REQUIRE_FALSE( w1.ready() );
    REQUIRE_FALSE( l.try_wait() );
    REQUIRE( l.counter() == 5 );
    wait<> w2 = l.arrive_and_wait( 3 );
    REQUIRE( l.counter() == 2 );
    run_all_cpp_coroutines();
    REQUIRE( l.counter() == 2 );
    REQUIRE_FALSE( w2.ready() );
    REQUIRE_FALSE( w1.ready() );
    REQUIRE_FALSE( l.try_wait() );
    REQUIRE( l.counter() == 2 );
    l.count_down( 2 );
    REQUIRE( l.counter() == 0 );
    run_all_cpp_coroutines();
    REQUIRE( l.counter() == 0 );
    REQUIRE( w1.ready() );
    REQUIRE( w2.ready() );
    REQUIRE( l.try_wait() );
    REQUIRE( l.Wait().ready() );
    REQUIRE( l.Wait().ready() );
    REQUIRE( l.counter() == 0 );
  }
}

} // namespace
} // namespace rn::co
