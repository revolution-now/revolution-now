/****************************************************************
**co-combinator.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-24.
*
* Description: Unit tests for the src/co-combinator.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/co-combinator.hpp"
#include "src/co-registry.hpp"
#include "src/waitable-coro.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn::co {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[co-combinator] any" ) {
  waitable_promise<> p1, p2;
  auto f1 = [p1]() -> waitable<> { co_await p1.waitable(); };
  auto f2 = [p2]() -> waitable<> { co_await p2.waitable(); };
  waitable<> w1 = f1();
  waitable<> w2 = f2();
  waitable<> w  = any( f1(), f2() );
  REQUIRE( !w.ready() );
  SECTION( "first" ) {
    p1.finish();
    REQUIRE( !w.ready() );
  }
  SECTION( "second" ) {
    p2.finish();
    REQUIRE( !w.ready() );
  }
  SECTION( "both" ) {
    p1.finish();
    p2.finish();
    REQUIRE( !w.ready() );
  }
  run_all_coroutines();
  REQUIRE( w.ready() );
}

TEST_CASE( "[co-combinator] all" ) {
  waitable_promise<> p1;
  waitable_promise<> p2;
  waitable_promise<> p3;

  // Add an extra level of coroutine indirection here to make
  // this test more juicy.
  waitable<> w1 = []( waitable_promise<> p ) -> waitable<> {
    co_await p.waitable();
  }( p1 );
  waitable<> w2 = []( waitable_promise<> p ) -> waitable<> {
    co_await p.waitable();
  }( p2 );
  waitable<> w3 = []( waitable_promise<> p ) -> waitable<> {
    co_await p.waitable();
  }( p3 );
  auto ss1 = w1.shared_state();
  auto ss2 = w2.shared_state();
  auto ss3 = w3.shared_state();

  // This is an "all" function.
  waitable<> w = []( waitable<> w1, waitable<> w2,
                     waitable<> w3 ) -> waitable<> {
    co_await std::move( w1 );
    co_await std::move( w2 );
    co_await std::move( w3 );
  }( std::move( w1 ), std::move( w2 ), std::move( w3 ) );

  SECTION( "run to completion" ) {
    run_all_coroutines();
    REQUIRE( !ss1->has_value() );
    REQUIRE( !ss2->has_value() );
    REQUIRE( !ss3->has_value() );
    REQUIRE( !w.ready() );
    p1.finish();
    run_all_coroutines();
    REQUIRE( ss1->has_value() );
    REQUIRE( !w.ready() );
    p3.finish();
    run_all_coroutines();
    REQUIRE( ss3->has_value() );
    REQUIRE( !w.ready() );
    p2.finish();
    run_all_coroutines();
    REQUIRE( ss2->has_value() );
    REQUIRE( w.ready() );
    REQUIRE( ss1->has_value() );
    REQUIRE( ss2->has_value() );
    REQUIRE( ss3->has_value() );
  }
  SECTION( "cancellation scheduled" ) {
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p1.finish();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p2.finish();
    REQUIRE( !w.ready() );
    // don't run all coroutines here -- keep it queued.
    w.cancel();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p3.finish();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( ss1->has_value() );
    REQUIRE( !ss2->has_value() );
    REQUIRE( !ss3->has_value() );
  }
  SECTION( "cancellation" ) {
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p1.finish();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p2.finish();
    run_all_coroutines();
    w.cancel();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p3.finish();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( ss1->has_value() );
    REQUIRE( ss2->has_value() );
    REQUIRE( !ss3->has_value() );
  }
}

TEST_CASE( "[co-combinator] with_cancel" ) {
  waitable_promise<int> p1;
  waitable_promise<>    p2;

  // Add an extra level of coroutine indirection here to make
  // this test more juicy.
  waitable<int> w1 =
      []( waitable_promise<int> p ) -> waitable<int> {
    co_return co_await p.waitable();
  }( p1 );
  waitable<> w2 = []( waitable_promise<> p ) -> waitable<> {
    co_await p.waitable();
  }( p2 );
  auto ss1 = w1.shared_state();
  auto ss2 = w2.shared_state();

  SECTION( "w1 finishes first" ) {
    {
      waitable<maybe<int>> w =
          with_cancel( std::move( w1 ), std::move( w2 ) );
      run_all_coroutines();
      REQUIRE( !w.ready() );
      p1.set_value( 5 );
      run_all_coroutines();
      REQUIRE( w.ready() );
      REQUIRE( w.get().has_value() );
      REQUIRE( w.get() == 5 );
      // At this point, `w` should go out of scope which should
      // free the (lambda) coroutine, which will free w1 and w2,
      // which will send cancellation signals to their shared
      // states, which should then prevent a p2.finish() from
      // propagating to ss2 since we have one layer of (then can-
      // celled) coroutine between p2 and ss2.
    }
    // Verify cancellation.
    p2.finish();
    run_all_coroutines();
    REQUIRE( !ss2->has_value() );
  }
  SECTION( "w2 finishes first" ) {
    waitable<maybe<int>> w =
        with_cancel( std::move( w1 ), std::move( w2 ) );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p2.finish();
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( !w.get().has_value() );
  }
  SECTION( "both (p1 first)" ) {
    waitable<maybe<int>> w =
        with_cancel( std::move( w1 ), std::move( w2 ) );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p1.set_value( 5 );
    p2.finish();
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( w.get().has_value() );
    REQUIRE( w.get() == 5 );
  }
  SECTION( "both (p2 first)" ) {
    waitable<maybe<int>> w =
        with_cancel( std::move( w1 ), std::move( w2 ) );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p2.finish();
    p1.set_value( 5 );
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( !w.get().has_value() );
  }
}

TEST_CASE( "[co-combinator] latch" ) {
  latch      l;
  waitable<> w1 = l.waitable();
  waitable<> w2 = l.waitable();
  REQUIRE( !w1.ready() );
  REQUIRE( !w2.ready() );
  l.set();
  REQUIRE( w1.ready() );
  REQUIRE( w2.ready() );
  l.reset();
  REQUIRE( w1.ready() );
  REQUIRE( w2.ready() );
  w1 = l.waitable();
  w2 = l.waitable();
  REQUIRE( !w1.ready() );
  REQUIRE( !w2.ready() );
  l.set();
  REQUIRE( w1.ready() );
  REQUIRE( w2.ready() );
  l.set();
  REQUIRE( w1.ready() );
  REQUIRE( w2.ready() );
}

TEST_CASE( "[co-combinator] ticker" ) {
  ticker t;
  t.tick();
  waitable<> w1 = t.wait();
  waitable<> w2 = t.wait();
  REQUIRE( !w1.ready() );
  REQUIRE( !w2.ready() );
  t.tick();
  REQUIRE( w1.ready() );
  REQUIRE( w2.ready() );
  t.tick();
  REQUIRE( w1.ready() );
  REQUIRE( w2.ready() );
  w1 = t.wait();
  w2 = t.wait();
  REQUIRE( !w1.ready() );
  REQUIRE( !w2.ready() );
  t.tick();
  REQUIRE( w1.ready() );
  REQUIRE( w2.ready() );
}

TEST_CASE( "[co-combinator] stream" ) {
  stream<int> s;
  waitable    w = s.next();
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  s.send( 5 );
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  s.update();
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == 5 );
  w = s.next();
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  s.send( 7 );
  s.send( 6 );
  s.send( 5 );
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  s.update();
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == 7 );
  w = s.next();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == 6 );
  w = s.next();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == 5 );
  w = s.next();
  REQUIRE( !w.ready() );
  s.send( 4 );
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  s.update();
  REQUIRE( !w.ready() );
  w.cancel();
  run_all_coroutines();
  REQUIRE( !w.ready() );
  w = s.next();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == 4 );
}

TEST_CASE( "[co-combinator] finite_stream" ) {
  finite_stream<int> s;
  waitable           w = s.next();
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  s.send( 5 );
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  s.update();
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == 5 );
  w = s.next();
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  s.send( 7 );
  s.send( 6 );
  s.send( 5 );
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  s.update();
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == 7 );
  w = s.next();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == 6 );
  w = s.next();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == 5 );
  w = s.next();
  REQUIRE( !w.ready() );
  s.send( 4 );
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  s.update();
  REQUIRE( !w.ready() );
  w.cancel();
  run_all_coroutines();
  REQUIRE( !w.ready() );
  w = s.next();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == 4 );

  // End it.
  w = s.next();
  REQUIRE( !w.ready() );
  s.finish();
  s.send( 9 );
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  s.update();
  run_all_coroutines();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == nothing );
  w = s.next();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == nothing );
}

} // namespace
} // namespace rn::co
