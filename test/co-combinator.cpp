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
#include "base/scope-exit.hpp"

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
    REQUIRE( w.get().index() == 0 );
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
    REQUIRE( w.get().index() == 1 );
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
    REQUIRE( w.get().index() == 2 );
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
    REQUIRE( w.get().index() == 0 );
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
    REQUIRE( w.get().index() == 2 );
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

TEST_CASE( "[waitable] abort with any" ) {
  waitable_promise<> p1;
  waitable_promise<> p2;
  waitable_promise<> p3;
  waitable<>         w =
      co::any( p1.waitable(), p2.waitable(), p3.waitable() );
  REQUIRE( !w.ready() );
  REQUIRE( !w.aborted() );

  p2.abort();
  REQUIRE( !w.ready() );
  REQUIRE( !w.aborted() );

  p3.abort();
  REQUIRE( !w.ready() );
  REQUIRE( !w.aborted() );

  p1.abort();
  REQUIRE( !w.ready() );
  REQUIRE( w.aborted() );
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
  co_await co::abort;
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

TEST_CASE( "[waitable] abort with various combinators" ) {
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
    REQUIRE( places == "kcfgiabBAjJIGFClLK" );
  }
  SECTION( "sanity check - cancellation" ) {
    run_all_coroutines();
    REQUIRE( !w.ready() );
    w.cancel();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( places == "kcfgiaAIGFCK" );
  }
  SECTION( "get_int1, get_int2, get_int3 all abort" ) {
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( !w.aborted() );
    // First let get_int1 abort via co::abort.
    get_int1_p.finish();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( !w.aborted() );
    REQUIRE( places == "kcfgiadDC" );
    // Now let get_int2 abort manually.
    get_int2_p.abort();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( !w.aborted() );
    REQUIRE( places == "kcfgiadDCGF" );
    // Now let get_int3 abort manually.
    int_stream.abort();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( places == "kcfgiadDCGFAIK" );

    // Now finally we have the root object aborted.
    REQUIRE( w.aborted() );

    // Now cancel w, just to make sure nothing goes wrong.
    w.cancel();
    run_all_coroutines();
    REQUIRE( !w.ready() );
    REQUIRE( !w.aborted() );
    REQUIRE( places == "kcfgiadDCGFAIK" );
  }
}

} // namespace
} // namespace rn::co
