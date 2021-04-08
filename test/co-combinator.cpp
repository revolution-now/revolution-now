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
  waitable<> w  = any( w1, w2 );
  REQUIRE( !w.ready() );
  SECTION( "first" ) {
    p1.finish();
    run_all_coroutines();
    REQUIRE( w1.ready() );
    REQUIRE( !w2.ready() );
  }
  SECTION( "second" ) {
    p2.finish();
    run_all_coroutines();
    REQUIRE( !w1.ready() );
    REQUIRE( w2.ready() );
  }
  SECTION( "both" ) {
    p1.finish();
    run_all_coroutines();
    p2.finish();
    run_all_coroutines();
    REQUIRE( w1.ready() );
    // Not ready because w2 has been cancelled because w1 was
    // ready first.
    REQUIRE( !w2.ready() );
    run_all_coroutines();
    REQUIRE( w1.ready() );
    REQUIRE( !w2.ready() );
  }
  REQUIRE( w.ready() );
}

TEST_CASE( "[co-combinator] vector any" ) {
  vector<waitable_promise<>> ps;
  ps.resize( 10 );
  vector<waitable<>> ws;
  for( auto& p : ps )
    // Use an extra layer of coroutine here so that we have some-
    // thing to cancel. If we don't do this, then we won't really
    // be able to verify that anything cancelled (later in this
    // test case) which we do by trying to set values on them
    // after they are cancelled and verifying that it has no ef-
    // fect. Without the extra layer of coroutine, setting the
    // values on them would make them ready.
    ws.push_back( []( waitable_promise<> p ) -> waitable<> {
      co_await p.waitable();
    }( p ) );
  waitable<> w = any( ws );
  REQUIRE( !w.ready() );

  for( int i = 0; i < 10; ++i ) //
    REQUIRE( !ws[i].ready() );

  ps[5].finish();
  REQUIRE( !w.ready() );
  run_all_coroutines();
  REQUIRE( w.ready() );

  // Make sure that only one is ready.
  for( int i = 0; i < 10; ++i ) {
    if( i == 5 )
      REQUIRE( ws[i].ready() );
    else
      REQUIRE( !ws[i].ready() );
  }

  // Try to set the ones that were cancelled.
  for( int i = 0; i < 10; ++i ) //
    if( i != 5 )                //
      ps[i].finish();
  run_all_coroutines();

  // Still only one ready, since the others were cancelled before
  // they were set.
  for( int i = 0; i < 10; ++i ) {
    if( i == 5 )
      REQUIRE( ws[i].ready() );
    else
      REQUIRE( !ws[i].ready() );
  }
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

  waitable<> w = all( w1, w2, w3 );

  SECTION( "run to completion" ) {
    run_all_coroutines();
    REQUIRE( !w1.ready() );
    REQUIRE( !w2.ready() );
    REQUIRE( !w3.ready() );
    REQUIRE( !w.ready() );
    p1.finish();
    run_all_coroutines();
    REQUIRE( w1.ready() );
    REQUIRE( !w.ready() );
    p3.finish();
    run_all_coroutines();
    REQUIRE( w3.ready() );
    REQUIRE( !w.ready() );
    p2.finish();
    run_all_coroutines();
    REQUIRE( w2.ready() );
    REQUIRE( w.ready() );
    REQUIRE( w1.ready() );
    REQUIRE( w2.ready() );
    REQUIRE( w3.ready() );
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
    REQUIRE( w1.ready() );
    REQUIRE( !w2.ready() );
    REQUIRE( !w3.ready() );
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
    REQUIRE( w1.ready() );
    REQUIRE( w2.ready() );
    REQUIRE( !w3.ready() );
  }
}

TEST_CASE( "[co-combinator] until_do" ) {
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

  waitable<int> w = until_do( w1, w2 );

  SECTION( "first finishes first" ) {
    run_all_coroutines();
    REQUIRE( !w1.ready() );
    REQUIRE( !w2.ready() );
    REQUIRE( !w.ready() );
    p1.set_value( 5 );
    run_all_coroutines();
    REQUIRE( w1.ready() );
    REQUIRE( w1.get() == 5 );
    REQUIRE( !w2.ready() );
    REQUIRE( w.ready() );
    REQUIRE( w.get() == 5 );
    // Verify cancellation.
    p2.finish();
    run_all_coroutines();
    REQUIRE( !w2.ready() );
  }
  SECTION( "background finishes first" ) {
    run_all_coroutines();
    REQUIRE( !w1.ready() );
    REQUIRE( !w2.ready() );
    REQUIRE( !w.ready() );
    p2.finish();
    run_all_coroutines();
    REQUIRE( !w1.ready() );
    REQUIRE( w2.ready() );
    REQUIRE( !w.ready() );
    p1.set_value( 5 );
    run_all_coroutines();
    REQUIRE( w1.ready() );
    REQUIRE( w1.get() == 5 );
    REQUIRE( w2.ready() );
    REQUIRE( w.ready() );
    REQUIRE( w.get() == 5 );
  }
  SECTION( "both" ) {
    run_all_coroutines();
    REQUIRE( !w1.ready() );
    REQUIRE( !w2.ready() );
    REQUIRE( !w.ready() );
    p1.set_value( 5 );
    p2.finish();
    run_all_coroutines();
    REQUIRE( w1.ready() );
    REQUIRE( w1.get() == 5 );
    REQUIRE( w2.ready() );
    REQUIRE( w.ready() );
    REQUIRE( w.get() == 5 );
  }
}

} // namespace
} // namespace rn::co
