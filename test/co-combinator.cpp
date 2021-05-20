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

// base
#include "base/lambda.hpp"
#include "base/scope-exit.hpp"

// Must be last.
#include "catch-common.hpp"

// This allows Catch2 to print out any variant that has three al-
// ternatives, each of which would be formattable with fmt::for-
// mat.
FMT_TO_CATCH_T( ( T1, T2, T3 ), ::base::variant );

namespace rn::co {
namespace {

using namespace std;

using Catch::Contains;
using Catch::Equals;

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

TEST_CASE( "[co-combinator] first" ) {
  waitable_promise<int>    p1;
  waitable_promise<>       p2;
  waitable_promise<string> p3;

  waitable<int>    w1 = p1.waitable();
  waitable<>       w2 = p2.waitable();
  waitable<string> w3 = p3.waitable();

  SECTION( "w1 finishes first" ) {
    waitable<base::variant<int, monostate, string>> w = first(
        std::move( w1 ), std::move( w2 ), std::move( w3 ) );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p1.set_value( 5 );
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( w->index() == 0 );
    REQUIRE( w.get().get_if<int>() == 5 );
  }
  SECTION( "w2 finishes first" ) {
    waitable<base::variant<int, monostate, string>> w = first(
        std::move( w1 ), std::move( w2 ), std::move( w3 ) );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p2.set_value_emplace();
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( w->index() == 1 );
    REQUIRE( w.get().get_if<monostate>().has_value() );
  }
  SECTION( "w3 finishes first" ) {
    waitable<base::variant<int, monostate, string>> w = first(
        std::move( w1 ), std::move( w2 ), std::move( w3 ) );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p3.set_value( "hello" );
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( w->index() == 2 );
    REQUIRE( w.get().get_if<string>() == "hello" );
  }
  SECTION( "both p1 and p3 finish (p1 first)" ) {
    waitable<base::variant<int, monostate, string>> w = first(
        std::move( w1 ), std::move( w2 ), std::move( w3 ) );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p1.set_value( 5 );
    p3.set_value( "hello" );
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( w->index() == 0 );
    REQUIRE( w.get().get_if<int>() == 5 );
  }
  SECTION( "both p1 and p3 finish (p3 first)" ) {
    waitable<base::variant<int, monostate, string>> w = first(
        std::move( w1 ), std::move( w2 ), std::move( w3 ) );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p3.set_value( "hello" );
    p1.set_value( 5 );
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( w->index() == 2 );
    REQUIRE( w.get().get_if<string>() == "hello" );
  }
}

TEST_CASE( "[co-combinator] background" ) {
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
      waitable<int> w =
          background( std::move( w1 ), std::move( w2 ) );
      run_all_coroutines();
      REQUIRE( !w.ready() );
      p1.set_value( 5 );
      run_all_coroutines();
      REQUIRE( w.ready() );
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
  SECTION( "w2 finishes first, w1 does not finish" ) {
    waitable<int> w =
        background( std::move( w1 ), std::move( w2 ) );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p2.finish();
    run_all_coroutines();
    REQUIRE( !w.ready() );
  }
  SECTION( "both (p1 first)" ) {
    waitable<int> w =
        background( std::move( w1 ), std::move( w2 ) );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p1.set_value( 5 );
    p2.finish();
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( w.get() == 5 );
  }
  SECTION( "both (p2 first)" ) {
    waitable<int> w =
        background( std::move( w1 ), std::move( w2 ) );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    p2.finish();
    p1.set_value( 5 );
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( w.get() == 5 );
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
  REQUIRE( w.ready() );
  REQUIRE( w.get() == nothing );
  w = s.next();
  REQUIRE( w.ready() );
  REQUIRE( w.get() == nothing );
}

waitable<int> some_coroutine( waitable<int>&& w ) {
  co_return co_await std::move( w );
}

TEST_CASE( "[co-combinator] detect_suspend" ) {
  waitable_promise<int> p1, p2;
  waitable<int>         w1 = p1.waitable();
  waitable<int>         w2 = p2.waitable();
  p1.set_value( 5 );

  auto should_not_suspend =
      detect_suspend( some_coroutine( std::move( w1 ) ) );
  auto should_suspend =
      detect_suspend( some_coroutine( std::move( w2 ) ) );
  run_all_coroutines();

  REQUIRE( should_not_suspend.ready() );
  REQUIRE( !should_suspend.ready() );

  ResultWithSuspend<int> const& rws1 = should_not_suspend.get();
  REQUIRE( rws1.result == 5 );
  REQUIRE( rws1.suspended == false );

  p2.set_value( 7 );
  run_all_coroutines();

  REQUIRE( should_suspend.ready() );
  ResultWithSuspend<int> const& rws2 = should_suspend.get();
  REQUIRE( rws2.result == 7 );
  REQUIRE( rws2.suspended == true );
}

TEST_CASE( "[waitable] simple exception chained" ) {
  waitable_promise<> p1;
  waitable<>         w1 = p1.waitable();
  REQUIRE( !w1.ready() );

  waitable_promise<> p2;
  disjunctive_link_to_promise( w1, p2 );
  waitable<> w2 = p2.waitable();

  waitable_promise<> p3;
  disjunctive_link_to_promise( w2, p3 );
  waitable<> w3 = p3.waitable();

  SECTION( "no exception" ) {
    REQUIRE( !w3.ready() );
    REQUIRE( !w3.has_exception() );
    p1.set_value_emplace();
    REQUIRE( w3.ready() );
    REQUIRE( !w3.has_exception() );
    REQUIRE( w2.ready() );
    REQUIRE( !w2.has_exception() );
    REQUIRE( w1.ready() );
    REQUIRE( !w1.has_exception() );
  }
  SECTION( "with exception" ) {
    REQUIRE( !w3.ready() );
    REQUIRE( !w3.has_exception() );
    p1.set_exception( runtime_error( "test-failed" ) );
    REQUIRE( !w3.ready() );
    REQUIRE( w3.has_exception() );
    REQUIRE( !w2.ready() );
    REQUIRE( w2.has_exception() );
    REQUIRE( !w1.ready() );
    REQUIRE( w1.has_exception() );
  }
  SECTION( "exception twice" ) {
    REQUIRE( !w3.ready() );
    REQUIRE( !w3.has_exception() );
    p1.set_exception( runtime_error( "test-failed" ) );
    p1.set_exception( runtime_error( "test-failed" ) );
    REQUIRE( !w3.ready() );
    REQUIRE( w3.has_exception() );
    REQUIRE( !w2.ready() );
    REQUIRE( w2.has_exception() );
    REQUIRE( !w1.ready() );
    REQUIRE( w1.has_exception() );
  }
}

TEST_CASE( "[co-combinator] exception with any" ) {
  waitable_promise<> p1;
  waitable_promise<> p2;
  waitable_promise<> p3;
  waitable<>         w =
      co::any( p1.waitable(), p2.waitable(), p3.waitable() );
  REQUIRE( !w.ready() );
  REQUIRE( !w.has_exception() );

  p2.set_exception( runtime_error( "test-failed" ) );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  REQUIRE( w.has_exception() );

  // Subsequent exceptions have no effect.
  p3.set_exception( runtime_error( "test-failed" ) );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  REQUIRE( w.has_exception() );

  p1.set_exception( runtime_error( "test-failed" ) );
  run_all_coroutines();
  REQUIRE( !w.ready() );
  REQUIRE( w.has_exception() );
}

waitable_promise<> get_int1_p;
waitable_promise<> get_int2_p;
stream<int>        int_stream;
string             places;

#define LOG_PLACES( a, A ) \
  places += a;             \
  SCOPE_EXIT( places += A );

waitable<int> get_int_from_stream() {
  LOG_PLACES( 'a', 'A' );
  int n = co_await int_stream.next();
  LOG_PLACES( 'b', 'B' );
  co_return n;
}

waitable<int> get_int1() {
  get_int1_p = {};
  LOG_PLACES( 'c', 'C' );
  co_await get_int1_p.waitable();
  LOG_PLACES( 'd', 'D' );
  throw runtime_error( "test-failed" );
  LOG_PLACES( 'e', 'E' );
  co_return 5;
}

waitable<> get_int2() {
  LOG_PLACES( 'f', 'F' );
  co_await repeat( []() -> waitable<> {
    get_int2_p = {};
    LOG_PLACES( 'g', 'G' );
    co_await get_int2_p.waitable();
    LOG_PLACES( 'x', 'X' );
  } );
  LOG_PLACES( 'h', 'H' );
}

waitable<double> get_int3() {
  LOG_PLACES( 'i', 'I' );
  co_await get_int_from_stream();
  LOG_PLACES( 'j', 'J' );
  co_return 6.6;
}

waitable<int> get_int_from_some_combinators() {
  LOG_PLACES( 'k', 'K' );
  // Do these out of line so that we can control the precise or-
  // der, so that the test is deterministic.
  auto                            w1 = get_int1();
  auto                            w2 = get_int2();
  auto                            w3 = get_int3();
  variant<int, monostate, double> v  = co_await first(
      std::move( w1 ), std::move( w2 ), std::move( w3 ) );
  LOG_PLACES( 'l', 'L' );
  REQUIRE( v.index() == 2 );
  REQUIRE( get<2>( v ) == 6.6 );
  co_return 9;
}

TEST_CASE(
    "[co-combinator] exception with various combinators" ) {
  places.clear();
  get_int1_p      = {};
  get_int2_p      = {};
  int_stream      = {};
  waitable<int> w = get_int_from_some_combinators();
  REQUIRE( places == "kcfgia" );

  SECTION( "sanity check - run to completion" ) {
    run_all_coroutines();
    REQUIRE( !w.ready() );
    int_stream.send( 3 );
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( w.get() == 9 );
    // Depends on order of parameter destruction.
    REQUIRE_THAT( places, Equals( "kcfgiabBAjJIGFClLK" ) ||
                              Equals( "kcfgiabBAjJICGFlLK" ) );
  }
  SECTION( "sanity check - cancellation" ) {
    run_all_coroutines();
    REQUIRE( !w.ready() );
    w.cancel();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    // Depends on order of parameter destruction.
    REQUIRE_THAT( places, Equals( "kcfgiaAIGFCK" ) ||
                              Equals( "kcfgiaCGFAIK" ) );
  }
  SECTION( "get_int1, get_int2, get_int3 all throw" ) {
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );
    SECTION( "get_int3 first" ) {
      // First, have get_int3 throw exception manually.
      int_stream.set_exception();
      run_all_coroutines();
      REQUIRE( !w.ready() );
      REQUIRE( w.has_exception() );
      // Depends on order of parameter destruction.
      REQUIRE_THAT( places, Equals( "kcfgiaAIGFCK" ) ||
                                Equals( "kcfgiaAICGFK" ) );
      // Subsequent exceptions should have no effect as those
      // branches have already been cancelled. Let get_int1
      // throw.
      get_int1_p.finish();
      run_all_coroutines();
      REQUIRE( !w.ready() );
      REQUIRE( w.has_exception() );
      // Depends on order of parameter destruction.
      REQUIRE_THAT( places, Equals( "kcfgiaAIGFCK" ) ||
                                Equals( "kcfgiaAICGFK" ) );
      get_int2_p.set_exception( runtime_error( "test-failed" ) );
      run_all_coroutines();
      REQUIRE( !w.ready() );
      REQUIRE( w.has_exception() );
      // Depends on order of parameter destruction.
      REQUIRE_THAT( places, Equals( "kcfgiaAIGFCK" ) ||
                                Equals( "kcfgiaAICGFK" ) );
    }
    SECTION( "get_int2 first" ) {
      // Let get_int2 throw manually.
      get_int2_p.set_exception( runtime_error( "test-failed" ) );
      run_all_coroutines();
      REQUIRE( !w.ready() );
      REQUIRE( w.has_exception() );
      // Depends on order of parameter destruction.
      REQUIRE_THAT( places, Equals( "kcfgiaGFAICK" ) ||
                                Equals( "kcfgiaGFCAIK" ) );
      // Subsequent exceptions should have no effect as those
      // branches have already been cancelled.
      get_int1_p.finish(); // causes exception.
      run_all_coroutines();
      REQUIRE( !w.ready() );
      REQUIRE( w.has_exception() );
      // Depends on order of parameter destruction.
      REQUIRE_THAT( places, Equals( "kcfgiaGFAICK" ) ||
                                Equals( "kcfgiaGFCAIK" ) );
      // Let get_int3 throw.
      int_stream.set_exception();
      run_all_coroutines();
      REQUIRE( !w.ready() );
      REQUIRE( w.has_exception() );
      // Depends on order of parameter destruction.
      REQUIRE_THAT( places, Equals( "kcfgiaGFAICK" ) ||
                                Equals( "kcfgiaGFCAIK" ) );
    }
    SECTION( "get_int1 first" ) {
      get_int1_p.finish(); // causes exception.
      run_all_coroutines();
      REQUIRE( !w.ready() );
      REQUIRE( w.has_exception() );
      // Depends on order of parameter destruction.
      REQUIRE_THAT( places, Equals( "kcfgiadDCAIGFK" ) ||
                                Equals( "kcfgiadDCGFAIK" ) );
      // Subsequent exceptions should have no effect as those
      // branches have already been cancelled.
      // Let get_int3 throw.
      int_stream.set_exception();
      run_all_coroutines();
      REQUIRE( !w.ready() );
      REQUIRE( w.has_exception() );
      // Depends on order of parameter destruction.
      REQUIRE_THAT( places, Equals( "kcfgiadDCAIGFK" ) ||
                                Equals( "kcfgiadDCGFAIK" ) );
      // Let get_int2 throw manually.
      get_int2_p.set_exception( runtime_error( "test-failed" ) );
      run_all_coroutines();
      REQUIRE( !w.ready() );
      REQUIRE( w.has_exception() );
      // Depends on order of parameter destruction.
      REQUIRE_THAT( places, Equals( "kcfgiadDCAIGFK" ) ||
                                Equals( "kcfgiadDCGFAIK" ) );
    }

    // Now cancel w, just to make sure nothing goes wrong.
    w.cancel();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );
  }
}

waitable_promise<int> wp;

waitable<int> throwing_coro( bool should_throw,
                             bool throw_eager ) {
  if( should_throw && throw_eager )
    throw runtime_error( "first" );
  int n = co_await wp.waitable();
  if( should_throw ) throw runtime_error( "second" );
  co_return n;
}

TEST_CASE( "[co-combinator] try" ) {
  wp = {};
  string what;
  auto   catcher = [&]( runtime_error const& e ) {
    what = e.what();
  };

  SECTION( "eager throw" ) {
    waitable<maybe<int>> w = co::try_<runtime_error>(
        LC0( throwing_coro( true, true ) ), catcher );
    REQUIRE( what == "" );
    run_all_coroutines();
    REQUIRE( what == "first" );
    REQUIRE( w.ready() );
    REQUIRE( !w.has_exception() );
    REQUIRE( w.get() == nothing );
  }
  SECTION( "lazy throw" ) {
    waitable<maybe<int>> w = co::try_<runtime_error>(
        LC0( throwing_coro( true, false ) ), catcher );
    REQUIRE( what == "" );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );
    REQUIRE( what == "" );
    wp.set_value( 9 );
    run_all_coroutines();
    REQUIRE( what == "second" );
    REQUIRE( w.ready() );
    REQUIRE( !w.has_exception() );
    REQUIRE( w.get() == nothing );
  }
  SECTION( "no throw" ) {
    waitable<maybe<int>> w = co::try_<runtime_error>(
        L0( throwing_coro( false, false ) ), catcher );
    REQUIRE( what == "" );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );
    REQUIRE( what == "" );
    wp.set_value( 9 );
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( !w.has_exception() );
    REQUIRE( what == "" );
    REQUIRE( w.get() == 9 );
  }
}

waitable<int> wait_on_stream( co::stream<int>& s ) {
  int n = co_await s.next(); // #1
  int m = co_await s.next(); // #2
  int o = co_await s.next(); // #3
  co_return n + m + o;
}

waitable<int> wait_on_finite_stream(
    co::finite_stream<int>& s ) {
  maybe<int> n = co_await s.next(); // #1
  REQUIRE( n.has_value() );
  maybe<int> m = co_await s.next(); // #2
  REQUIRE( m.has_value() );
  maybe<int> o = co_await s.next(); // #3
  REQUIRE( o.has_value() );
  co_return *n + *m + *o;
}

// Ensure that:
//
//   1. streams are cancellable, and
//   2. streams can be used again after cancellation without
//      losing any values in the stream.
//
TEST_CASE( "[co-combinator] stream: cancel and reuse" ) {
  int const kFirst  = 1;
  int const kSecond = 10;
  int const kThird  = 100;
  int const kFourth = 1000;

  SECTION( "stream" ) {
    co::stream<int> s;
    {
      waitable<int> w = wait_on_stream( s );
      run_all_coroutines();
      REQUIRE( !w.ready() );
      s.send( kFirst );
      run_all_coroutines();
      REQUIRE( !w.ready() );
      // Now send the remainder of the values, but instead of
      // then running all coroutines, just let w get cancelled,
      // so that we can later verify that those values are still
      // available in the stream.
      s.send( kSecond );
      s.send( kThird );
      REQUIRE( !w.ready() );
    }
    // w has now been cancelled.
    waitable<int> w = wait_on_stream( s );
    REQUIRE( !w.ready() );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    // At this point, we should be waiting at #3.
    s.send( kFourth );
    REQUIRE( !w.ready() );
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( *w == kSecond + kThird + kFourth );
  }
  SECTION( "finite_stream" ) {
    co::finite_stream<int> s;
    {
      waitable<int> w = wait_on_finite_stream( s );
      run_all_coroutines();
      REQUIRE( !w.ready() );
      s.send( kFirst );
      run_all_coroutines();
      REQUIRE( !w.ready() );
      // Now send the remainder of the values, but instead of
      // then running all coroutines, just let w get cancelled,
      // so that we can later verify that those values are still
      // available in the stream.
      s.send( kSecond );
      s.send( kThird );
      REQUIRE( !w.ready() );
    }
    // w has now been cancelled.
    waitable<int> w = wait_on_finite_stream( s );
    REQUIRE( !w.ready() );
    run_all_coroutines();
    REQUIRE( !w.ready() );
    // At this point, we should be waiting at #3.
    s.send( kFourth );
    REQUIRE( !w.ready() );
    run_all_coroutines();
    REQUIRE( w.ready() );
    REQUIRE( *w == kSecond + kThird + kFourth );

    // Now let's end the finite stream while someone is waiting
    // on it, then cancel it, and make sure it is still ended.
    {
      waitable<maybe<int>> w = s.next();
      REQUIRE( !w.ready() );
      run_all_coroutines();
      REQUIRE( !w.ready() );
      s.finish();
      REQUIRE( !w.ready() );
      // w gets cancelled now.
    }
    waitable<maybe<int>> w2 = s.next();
    REQUIRE( w2.ready() );
    REQUIRE( *w2 == nothing );

    waitable<maybe<int>> w3 = s.next();
    REQUIRE( w3.ready() );
    REQUIRE( *w3 == nothing );
  }
}

TEST_CASE( "[co-combinator] interleave" ) {
  co::stream<int> s1;
  co::stream<int> s2;
  co::stream<int> s3;

  SECTION( "send all values first" ) {
    co::interleave il( s1, s2, s3 );
    vector<int>    found;

    for( int i = 3; i < 6; ++i ) s1.send( i );
    for( int i = 0; i < 3; ++i ) s2.send( i );
    for( int i = 6; i < 9; ++i ) s3.send( i );

    while( true ) {
      waitable<base::variant<int, int, int>> w = il.next();
      run_all_coroutines();
      if( !w.ready() ) break;
      found.push_back( std::visit( L( _ ), *w ) );
    }

    vector<int> expected{ 3, 4, 5, 0, 1, 2, 6, 7, 8 };
    REQUIRE_THAT( found, Equals( expected ) );
  }
  SECTION( "send then query, per stream" ) {
    co::interleave il( s1, s2, s3 );
    vector<int>    found;

    for( int i = 3; i < 6; ++i ) s1.send( i );
    while( true ) {
      waitable<base::variant<int, int, int>> w = il.next();
      run_all_coroutines();
      if( !w.ready() ) break;
      REQUIRE( w->index() == 0 );
      found.push_back( std::visit( L( _ ), *w ) );
    }

    for( int i = 0; i < 3; ++i ) s2.send( i );
    while( true ) {
      waitable<base::variant<int, int, int>> w = il.next();
      run_all_coroutines();
      if( !w.ready() ) break;
      REQUIRE( w->index() == 1 );
      found.push_back( std::visit( L( _ ), *w ) );
    }

    for( int i = 6; i < 9; ++i ) s3.send( i );
    while( true ) {
      waitable<base::variant<int, int, int>> w = il.next();
      run_all_coroutines();
      if( !w.ready() ) break;
      REQUIRE( w->index() == 2 );
      found.push_back( std::visit( L( _ ), *w ) );
    }

    vector<int> expected{ 3, 4, 5, 0, 1, 2, 6, 7, 8 };
    REQUIRE_THAT( found, Equals( expected ) );
  }
  SECTION( "send then query, per stream" ) {
    co::interleave il( s1, s2, s3 );
    vector<int>    found;
    size_t         idx = 0;

    s1.send( 3 );
    s2.send( 0 );
    s3.send( 6 );
    idx = 0;
    while( true ) {
      waitable<base::variant<int, int, int>> w = il.next();
      run_all_coroutines();
      if( !w.ready() ) break;
      REQUIRE( w->index() == idx++ );
      found.push_back( std::visit( L( _ ), *w ) );
    }

    s1.send( 4 );
    s2.send( 1 );
    s3.send( 7 );
    idx = 0;
    while( true ) {
      waitable<base::variant<int, int, int>> w = il.next();
      run_all_coroutines();
      if( !w.ready() ) break;
      REQUIRE( w->index() == idx++ );
      found.push_back( std::visit( L( _ ), *w ) );
    }

    s1.send( 5 );
    s2.send( 2 );
    s3.send( 8 );
    idx = 0;
    while( true ) {
      waitable<base::variant<int, int, int>> w = il.next();
      run_all_coroutines();
      if( !w.ready() ) break;
      REQUIRE( w->index() == idx++ );
      found.push_back( std::visit( L( _ ), *w ) );
    }

    vector<int> expected{ 3, 0, 6, 4, 1, 7, 5, 2, 8 };
    REQUIRE_THAT( found, Equals( expected ) );
  }
  SECTION(
      "reconstruct interleave object multiple times on same "
      "streams" ) {
    // This test works because we are only destroying the co::in-
    // terleave objects when the output stream has been ex-
    // hausted.
    vector<int> found;
    {
      co::interleave il( s1, s2, s3 );

      s1.send( 3 );
      s2.send( 0 );
      s3.send( 6 );
      while( true ) {
        waitable<base::variant<int, int, int>> w = il.next();
        run_all_coroutines();
        if( !w.ready() ) break;
        found.push_back( std::visit( L( _ ), *w ) );
      }
    }

    {
      co::interleave il( s1, s2, s3 );
      s1.send( 4 );
      s2.send( 1 );
      s3.send( 7 );
      while( true ) {
        waitable<base::variant<int, int, int>> w = il.next();
        run_all_coroutines();
        if( !w.ready() ) break;
        found.push_back( std::visit( L( _ ), *w ) );
      }
    }

    {
      co::interleave il( s1, s2, s3 );
      s1.send( 5 );
      s2.send( 2 );
      s3.send( 8 );
      while( true ) {
        waitable<base::variant<int, int, int>> w = il.next();
        run_all_coroutines();
        if( !w.ready() ) break;
        found.push_back( std::visit( L( _ ), *w ) );
      }
    }

    vector<int> expected{ 3, 0, 6, 4, 1, 7, 5, 2, 8 };
    REQUIRE_THAT( found, Equals( expected ) );
  }
}

TEST_CASE( "[co-combinator] interleave different types" ) {
  co::stream<int>    s1;
  co::stream<double> s2;
  co::stream<string> s3;

  co::interleave il( s1, s2, s3 );

  vector<base::variant<int, double, string>> found;

  for( int i = 3; i < 6; ++i ) s1.send( i );
  for( double d = 0.0; d < 3.0; d += 1.0 ) s2.send( d );
  for( string s = "x"; s.size() < 4; s += "x" ) s3.send( s );

  while( true ) {
    waitable<base::variant<int, double, string>> w = il.next();
    run_all_coroutines();
    if( !w.ready() ) break;
    found.push_back( *w );
  }

  vector<base::variant<int, double, string>> expected{
      { 3 },   { 4 },   { 5 },    { 0.0 },  { 1.0 },
      { 2.0 }, { "x" }, { "xx" }, { "xxx" } };

  REQUIRE_THAT( found, Equals( expected ) );
}

TEST_CASE( "[co-combinator] one shot stream adapter" ) {
  waitable_promise<int> p;
  auto shot = co::make_streamable( p.waitable() );
  run_all_coroutines();

  waitable<int> w1 = shot.next();
  run_all_coroutines();
  REQUIRE( !w1.ready() );
  p.set_value( 5 );
  run_all_coroutines();
  REQUIRE( w1.ready() );
  REQUIRE( *w1 == 5 );

  waitable<int> w2 = shot.next();
  run_all_coroutines();
  REQUIRE( !w2.ready() );
}

TEST_CASE(
    "[co-combinator] interleave with waitables via stream "
    "adapter" ) {
  waitable_promise<int> p1;
  waitable_promise<int> p2;

  // Convert the waitbales into things that have a Streamable in-
  // terface.
  auto shot1 = co::make_streamable( p1.waitable() );
  auto shot2 = co::make_streamable( p2.waitable() );

  co::interleave il( shot1, shot2 );

  SECTION( "send both first" ) {
    {
      waitable<base::variant<int, int>> w = il.next();
      REQUIRE( !w.ready() );
      p2.set_value( 5 );
      p1.set_value( 6 );
      run_all_coroutines();
      REQUIRE( w.ready() );
      REQUIRE( w->index() == 1 );
      REQUIRE( get<1>( *w ) == 5 );
    }
    {
      waitable<base::variant<int, int>> w = il.next();
      run_all_coroutines();
      REQUIRE( w.ready() );
      REQUIRE( w->index() == 0 );
      REQUIRE( get<0>( *w ) == 6 );
    }
  }
  SECTION( "send one at a time" ) {
    {
      waitable<base::variant<int, int>> w = il.next();
      REQUIRE( !w.ready() );
      p2.set_value( 5 );
      run_all_coroutines();
      REQUIRE( w.ready() );
      REQUIRE( w->index() == 1 );
      REQUIRE( get<1>( *w ) == 5 );
    }
    {
      waitable<base::variant<int, int>> w = il.next();
      run_all_coroutines();
      REQUIRE( !w.ready() );
      p1.set_value( 6 );
      run_all_coroutines();
      REQUIRE( w.ready() );
      REQUIRE( w->index() == 0 );
      REQUIRE( get<0>( *w ) == 6 );
    }
    // Now we've exhausted the two waitables so we should not
    // have anything further.
    {
      waitable<base::variant<int, int>> w = il.next();
      run_all_coroutines();
      REQUIRE( !w.ready() );
    }
  }
}

} // namespace
} // namespace rn::co
