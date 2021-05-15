/****************************************************************
**waitable.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-08.
*
* Description: Unit tests for the waitable module.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "co-registry.hpp"
#include "waitable-coro.hpp"
#include "waitable.hpp"

// base
#include "base/scope-exit.hpp"

// Must be last.
#include "catch-common.hpp"

// C++ standard library
#include <queue>

FMT_TO_CATCH_T( ( T ), ::rn::waitable );
FMT_TO_CATCH_T( ( T ), ::rn::waitable_promise );

namespace rn {
namespace {

using namespace std;
using namespace rn;

using ::Catch::Equals;

// Unit test for waitable's default template parameter.
static_assert( is_same_v<waitable<>, waitable<monostate>> );

TEST_CASE( "[waitable] future api basic" ) {
  auto ss = make_shared<detail::waitable_shared_state<int>>();

  waitable<int> s_future( ss );

  REQUIRE( !s_future.ready() );

  ss->set( 3 );
  REQUIRE( s_future.ready() );
  REQUIRE( s_future.get() == 3 );
}

TEST_CASE( "[waitable] promise api basic api" ) {
  waitable_promise<int> s_promise;
  REQUIRE( !s_promise.has_value() );

  waitable<int> s_future = s_promise.waitable();
  REQUIRE( !s_future.ready() );

  s_promise.set_value( 3 );
  s_promise.set_value_if_not_set( 4 );
  REQUIRE( s_promise.has_value() );
  REQUIRE( s_future.ready() );
  REQUIRE( s_future.get() == 3 );
}

TEST_CASE( "[waitable] formatting" ) {
  waitable_promise<int> s_promise;
  REQUIRE( fmt::format( "{}", s_promise ) == "<empty>" );

  waitable<int> s_future = s_promise.waitable();
  REQUIRE( fmt::format( "{}", s_future ) == "<waiting>" );

  s_promise.set_value_if_not_set( 3 );
  REQUIRE( fmt::format( "{}", s_promise ) == "<ready>" );
  REQUIRE( fmt::format( "{}", s_future ) == "<ready>" );

  REQUIRE( s_future.get() == 3 );
  REQUIRE( fmt::format( "{}", s_promise ) == "<ready>" );
}

struct LogDestruction {
  LogDestruction( bool& b_ ) : b( &b_ ) {}
  ~LogDestruction() {
    if( b ) *b = true;
  }
  LogDestruction( LogDestruction const& ) = delete;
  LogDestruction( LogDestruction&& rhs ) noexcept
    : b( exchange( rhs.b, nullptr ) ) {}
  bool* b;
};

/****************************************************************
** Coroutines
*****************************************************************/
template<typename T>
using w_coro_promise = waitable_promise<T>;

queue<variant<w_coro_promise<int>, w_coro_promise<double>>>
    g_promises;

void deliver_promise() {
  struct Setter {
    void operator()( w_coro_promise<int>& p ) {
      p.set_value( 1 );
    }
    void operator()( w_coro_promise<double>& p ) {
      p.set_value( 2.2 );
    }
  };
  if( !g_promises.empty() ) {
    visit( Setter{}, g_promises.front() );
    g_promises.pop();
  }
}

waitable<int> waitable_int() {
  w_coro_promise<int> p;
  g_promises.emplace( p );
  return p.waitable();
}

waitable<double> waitable_double() {
  w_coro_promise<double> p;
  g_promises.emplace( p );
  return p.waitable();
}

waitable<int> waitable_sum() {
  co_return                     //
      co_await waitable_int() + //
      co_await waitable_int() + //
      co_await waitable_int();
}

template<typename Func, typename... Args>
auto co_invoke( Func&& func, Args... args )
    -> waitable<decltype( std::forward<Func>( func )(
        std::declval<typename Args::value_type>()... ) )> {
  co_return std::forward<Func>( func )(
      ( co_await std::move( args ) )... );
}

template<typename Func>
struct co_lift {
  Func func_;
  co_lift( Func&& func ) : func_( std::move( func ) ) {}
  co_lift( Func const& func ) : func_( func ) {}
  template<typename... Args>
  auto operator()( Args... args ) {
    return co_invoke( func_, std::move( args )... );
  }
};

waitable<string> waitable_string() {
  int    n = co_await waitable_sum();
  double d = co_await waitable_double();

  int m = co_await waitable_sum();
  for( int i = 0; i < m; ++i ) //
    d += co_await waitable_double();

  int sum = co_await co_lift{ std::plus<>{} }( waitable_sum(),
                                               waitable_sum() );

  auto f = [&]() -> waitable<int> {
    int res = co_await waitable_sum() *
              int( co_await waitable_double() );
    co_return res;
  };
  int z = co_await f() + sum;

  co_return to_string( n ) + "-" + to_string( z ) + "-" +
      to_string( d );
}

TEST_CASE( "[waitable] coro" ) {
  waitable<string> ws = waitable_string();
  int              i  = 0;
  while( !ws.ready() ) {
    ++i;
    deliver_promise();
    run_all_coroutines();
  }
  REQUIRE( ws.get() == "3-12-8.800000" );
  REQUIRE( i == 20 );
}

/****************************************************************
** Coroutine Cancellation
*****************************************************************/
vector<string> string_log;

struct LogDestructionStr {
  LogDestructionStr( string const& s_ ) : s( s_ ) {}
  ~LogDestructionStr() { string_log.push_back( "~~~: " + s ); }
  string s;
};

void log_str( string const& s ) {
  string_log.push_back( "run: " + s );
}

waitable_promise<string> p;
waitable_promise<int>    p0;
waitable_promise<int>    p1;

waitable<string> coro() {
  LogDestructionStr lds( "coro" );
  log_str( "coro" );
  return p.waitable();
}

waitable<int> coro0() {
  LogDestructionStr lds( "coro0" );
  log_str( "coro0" );
  return p0.waitable();
}

waitable<int> coro1() {
  LogDestructionStr lds( "coro1" );
  log_str( "coro1" );
  return p1.waitable();
}

waitable<string> coro2() {
  int n;
  {
    LogDestructionStr lds( "coro2-1" );
    log_str( "coro2-1" );
    n = co_await coro0();
  }
  LogDestructionStr lds1( "coro2-2" );
  log_str( "coro2-2" );
  string            res = to_string( n + co_await coro1() );
  LogDestructionStr lds2( "coro2-3" );
  log_str( "coro2-3" );
  co_return res;
}

waitable<string> coro3() {
  LogDestructionStr lds1( "coro3-1" );
  log_str( "coro3-1" );
  string            s = co_await coro2();
  LogDestructionStr lds2( "coro3-2" );
  log_str( "coro3-2" );
  s += co_await coro();
  LogDestructionStr lds3( "coro3-3" );
  log_str( "coro3-3" );
  co_return s + ".";
}

TEST_CASE( "[waitable] coro cancel" ) {
  p  = {};
  p0 = {};
  p1 = {};
  string_log.clear();

  waitable<string> ws = coro3();

  SECTION( "no cancel" ) {
    REQUIRE( !ws.ready() );
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p0.set_value( 5 );
    REQUIRE( number_of_queued_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_coroutines();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p1.set_value( 7 );
    REQUIRE( number_of_queued_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_coroutines();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p.set_value( "!" );
    REQUIRE( number_of_queued_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_coroutines();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( ws.ready() );
    // Cancelling a ready waitable should do nothing.
    ws.cancel();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( ws.ready() );

    REQUIRE( ws.get() == "12!." );
    vector<string> expected{
        "run: coro3-1", //
        "run: coro2-1", //
        "run: coro0",   //
        "~~~: coro0",   //
        "~~~: coro2-1", //
        "run: coro2-2", //
        "run: coro1",   //
        "~~~: coro1",   //
        "run: coro2-3", //
        "~~~: coro2-3", //
        "~~~: coro2-2", //
        "run: coro3-2", //
        "run: coro",    //
        "~~~: coro",    //
        "run: coro3-3", //
        "~~~: coro3-3", //
        "~~~: coro3-2", //
        "~~~: coro3-1", //
    };
    REQUIRE_THAT( string_log, Equals( expected ) );
  }

  SECTION( "cancel coro0 no schedule" ) {
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    ws.cancel();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );

    vector<string> expected{
        "run: coro3-1", //
        "run: coro2-1", //
        "run: coro0",   //
        "~~~: coro0",   //
        "~~~: coro2-1", //
        "~~~: coro3-1", //
    };
    REQUIRE_THAT( string_log, Equals( expected ) );
  }

  SECTION( "cancel coro0 with schedule" ) {
    REQUIRE( !ws.ready() );
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p0.set_value( 5 );
    REQUIRE( number_of_queued_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    ws.cancel();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );

    vector<string> expected{
        "run: coro3-1", //
        "run: coro2-1", //
        "run: coro0",   //
        "~~~: coro0",   //
        "~~~: coro2-1", //
        "~~~: coro3-1", //
    };
    REQUIRE_THAT( string_log, Equals( expected ) );
  }

  SECTION( "cancel coro1 no schedule" ) {
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p0.set_value( 5 );
    REQUIRE( number_of_queued_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_coroutines();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    ws.cancel();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );

    vector<string> expected{
        "run: coro3-1", //
        "run: coro2-1", //
        "run: coro0",   //
        "~~~: coro0",   //
        "~~~: coro2-1", //
        "run: coro2-2", //
        "run: coro1",   //
        "~~~: coro1",   //
        "~~~: coro2-2", //
        "~~~: coro3-1", //
    };
    REQUIRE_THAT( string_log, Equals( expected ) );
  }

  SECTION( "cancel coro1 with schedule" ) {
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p0.set_value( 5 );
    REQUIRE( number_of_queued_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_coroutines();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p1.set_value( 7 );
    REQUIRE( number_of_queued_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    ws.cancel();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );

    vector<string> expected{
        "run: coro3-1", //
        "run: coro2-1", //
        "run: coro0",   //
        "~~~: coro0",   //
        "~~~: coro2-1", //
        "run: coro2-2", //
        "run: coro1",   //
        "~~~: coro1",   //
        "~~~: coro2-2", //
        "~~~: coro3-1", //
    };
    REQUIRE_THAT( string_log, Equals( expected ) );
  }

  SECTION( "cancel coro no schedule" ) {
    REQUIRE( !ws.ready() );
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p0.set_value( 5 );
    REQUIRE( number_of_queued_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_coroutines();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p1.set_value( 7 );
    REQUIRE( number_of_queued_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_coroutines();
    ws.cancel();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );

    vector<string> expected{
        "run: coro3-1", //
        "run: coro2-1", //
        "run: coro0",   //
        "~~~: coro0",   //
        "~~~: coro2-1", //
        "run: coro2-2", //
        "run: coro1",   //
        "~~~: coro1",   //
        "run: coro2-3", //
        "~~~: coro2-3", //
        "~~~: coro2-2", //
        "run: coro3-2", //
        "run: coro",    //
        "~~~: coro",    //
        "~~~: coro3-2", //
        "~~~: coro3-1", //
    };
    REQUIRE_THAT( string_log, Equals( expected ) );
  }

  SECTION( "cancel coro with schedule" ) {
    REQUIRE( !ws.ready() );
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p0.set_value( 5 );
    REQUIRE( number_of_queued_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_coroutines();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p1.set_value( 7 );
    REQUIRE( number_of_queued_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_coroutines();
    REQUIRE( number_of_queued_coroutines() == 0 );
    p.set_value( "!" );
    REQUIRE( number_of_queued_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    ws.cancel();
    REQUIRE( number_of_queued_coroutines() == 0 );
    REQUIRE( !ws.ready() );

    vector<string> expected{
        "run: coro3-1", //
        "run: coro2-1", //
        "run: coro0",   //
        "~~~: coro0",   //
        "~~~: coro2-1", //
        "run: coro2-2", //
        "run: coro1",   //
        "~~~: coro1",   //
        "run: coro2-3", //
        "~~~: coro2-3", //
        "~~~: coro2-2", //
        "run: coro3-2", //
        "run: coro",    //
        "~~~: coro",    //
        "~~~: coro3-2", //
        "~~~: coro3-1", //
    };
    REQUIRE_THAT( string_log, Equals( expected ) );
  }
}

TEST_CASE( "[waitable] coro cancel by waitable out-of-scope" ) {
  p  = {};
  p0 = {};
  p1 = {};
  string_log.clear();

  SECTION( "cancel coro with scheduled" ) {
    {
      waitable<string> ws = coro3();

      REQUIRE( number_of_queued_coroutines() == 0 );
      REQUIRE( !ws.ready() );
      p0.set_value( 5 );
      REQUIRE( number_of_queued_coroutines() == 1 );
      REQUIRE( !ws.ready() );
      run_all_coroutines();
      REQUIRE( number_of_queued_coroutines() == 0 );
      REQUIRE( !ws.ready() );
      p1.set_value( 7 );
      REQUIRE( number_of_queued_coroutines() == 1 );
      REQUIRE( !ws.ready() );

      vector<string> expected{
          "run: coro3-1", //
          "run: coro2-1", //
          "run: coro0",   //
          "~~~: coro0",   //
          "~~~: coro2-1", //
          "run: coro2-2", //
          "run: coro1",   //
          "~~~: coro1",   //
      };
      REQUIRE_THAT( string_log, Equals( expected ) );
      // !! ws goes out of scope here and should get cancelled in
      // the process.
    }
    REQUIRE( number_of_queued_coroutines() == 0 );

    vector<string> expected{
        "run: coro3-1", //
        "run: coro2-1", //
        "run: coro0",   //
        "~~~: coro0",   //
        "~~~: coro2-1", //
        "run: coro2-2", //
        "run: coro1",   //
        "~~~: coro1",   //
        "~~~: coro2-2", //
        "~~~: coro3-1", //
    };
    REQUIRE_THAT( string_log, Equals( expected ) );
  }
  SECTION( "cancel coro no scheduled" ) {
    {
      waitable<string> ws = coro3();

      REQUIRE( number_of_queued_coroutines() == 0 );
      REQUIRE( !ws.ready() );
      p0.set_value( 5 );
      REQUIRE( number_of_queued_coroutines() == 1 );
      REQUIRE( !ws.ready() );
      run_all_coroutines();
      REQUIRE( number_of_queued_coroutines() == 0 );
      REQUIRE( !ws.ready() );
      p1.set_value( 7 );
      REQUIRE( number_of_queued_coroutines() == 1 );
      REQUIRE( !ws.ready() );
      run_all_coroutines();
      REQUIRE( number_of_queued_coroutines() == 0 );
      REQUIRE( !ws.ready() );

      vector<string> expected{
          "run: coro3-1", //
          "run: coro2-1", //
          "run: coro0",   //
          "~~~: coro0",   //
          "~~~: coro2-1", //
          "run: coro2-2", //
          "run: coro1",   //
          "~~~: coro1",   //
          "run: coro2-3", //
          "~~~: coro2-3", //
          "~~~: coro2-2", //
          "run: coro3-2", //
          "run: coro",    //
          "~~~: coro",    //
      };
      REQUIRE_THAT( string_log, Equals( expected ) );
      // !! ws goes out of scope here and should get cancelled in
      // the process.
    }
    REQUIRE( number_of_queued_coroutines() == 0 );

    vector<string> expected{
        "run: coro3-1", //
        "run: coro2-1", //
        "run: coro0",   //
        "~~~: coro0",   //
        "~~~: coro2-1", //
        "run: coro2-2", //
        "run: coro1",   //
        "~~~: coro1",   //
        "run: coro2-3", //
        "~~~: coro2-3", //
        "~~~: coro2-2", //
        "run: coro3-2", //
        "run: coro",    //
        "~~~: coro",    //
        "~~~: coro3-2", //
        "~~~: coro3-1", //
    };
    REQUIRE_THAT( string_log, Equals( expected ) );
  }
}

TEST_CASE( "[waitable] simple exception" ) {
  waitable_promise<> p;
  waitable<>         w = p.waitable();

  REQUIRE( !w.ready() );
  p.set_exception( runtime_error( "test" ) );
  REQUIRE( !w.ready() );
}

TEST_CASE( "[waitable] simple exception chained" ) {
  waitable_promise<> p1;
  waitable<>         w1 = p1.waitable();
  REQUIRE( !w1.ready() );

  waitable_promise<> p2;
  w1.link_to_promise( p2 );
  waitable<> w2 = p2.waitable();

  waitable_promise<> p3;
  w2.link_to_promise( p3 );
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

waitable_promise<> exception_p0;
waitable_promise<> exception_p1;
waitable_promise<> exception_p2;
string             places;

waitable<> exception_coro_early_level_2() {
  places += 'c';
  SCOPE_EXIT( places += 'C' );
  throw runtime_error( "test" );
  places += 'd';
  SCOPE_EXIT( places += 'D' );
}

waitable<> exception_coro_early_level_1() {
  places += 'a';
  SCOPE_EXIT( places += 'A' );
  waitable<> w = exception_coro_early_level_2();
  REQUIRE( w.has_exception() );
  co_await std::move( w );
  places += 'b';
  SCOPE_EXIT( places += 'B' );
}

TEST_CASE( "[waitable] exception coro early two_levels" ) {
  places.clear();

  waitable<> w = exception_coro_early_level_1();
  REQUIRE( places == "acCA" );
  REQUIRE( !w.ready() );
  REQUIRE( w.has_exception() );
}

waitable<> exception_coro_simple() {
  places += 'a';
  SCOPE_EXIT( places += 'A' );
  co_await exception_p0.waitable();
  places += 'b';
  SCOPE_EXIT( places += 'B' );
  throw runtime_error( "test" );
  places += 'c';
  SCOPE_EXIT( places += 'C' );
}

TEST_CASE( "[waitable] exception coro simple" ) {
  places.clear();
  exception_p0 = {};

  SECTION( "forward cancellation" ) {
    // We don't really need to test this here because it's cov-
    // ered in other tests, but just do it anyway.
    waitable<> w = exception_coro_simple();
    REQUIRE( places == "a" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    w.cancel();
    REQUIRE( places == "aA" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );
  }

  SECTION( "exception (backward cancellation)" ) {
    waitable<> w = exception_coro_simple();
    REQUIRE( places == "a" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_coroutines();
    REQUIRE( places == "abBA" );
    REQUIRE( !w.ready() );
    REQUIRE( w.has_exception() );
  }
}

waitable<> exception_0() {
  exception_p0 = {};
  places += 'l';
  SCOPE_EXIT( places += 'L' );
  co_await exception_p0.waitable();
  places += 'm';
  SCOPE_EXIT( places += 'M' );
}

waitable<> exception_2() {
  places += 'h';
  SCOPE_EXIT( places += 'H' );
  co_await exception_p1.waitable();
  places += 'i';
  SCOPE_EXIT( places += 'I' );
  throw runtime_error( "test" );
  places += 'j';
  SCOPE_EXIT( places += 'J' );
  co_await exception_p2.waitable();
  places += 'k';
  SCOPE_EXIT( places += 'K' );
}

waitable<> exception_1() {
  places += 'e';
  SCOPE_EXIT( places += 'E' );
  co_await exception_0();
  places += 'f';
  SCOPE_EXIT( places += 'F' );
  co_await exception_2();
  places += 'g';
  SCOPE_EXIT( places += 'G' );
}

waitable<> exception_coro_complex() {
  places += 'a';
  SCOPE_EXIT( places += 'A' );
  co_await exception_0();
  places += 'b';
  SCOPE_EXIT( places += 'B' );
  co_await exception_1();
  places += 'c';
  SCOPE_EXIT( places += 'C' );
  co_await exception_0();
  places += 'd';
  SCOPE_EXIT( places += 'D' );
}

TEST_CASE( "[waitable] exception coro complex" ) {
  places.clear();
  exception_p0 = {};
  exception_p1 = {};
  exception_p2 = {};

  SECTION( "cancelled, then exception via promise" ) {
    // We don't really need to test this here because it's cov-
    // ered in other tests, but just do it anyway.
    waitable<> w = exception_coro_complex();
    REQUIRE( places == "al" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_coroutines();
    REQUIRE( places == "almMLbel" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_coroutines();
    REQUIRE( places == "almMLbelmMLfh" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    w.cancel();
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    run_all_coroutines();
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    // Since it is already cancelled, an exception should not re-
    // ally do anything, but we just want to make sure that it
    // doesn't crash and doesn't change anything.
    exception_p0.set_exception( runtime_error( "test-failed" ) );
    run_all_coroutines();
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p1.set_exception( runtime_error( "test-failed" ) );
    run_all_coroutines();
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p2.set_exception( runtime_error( "test-failed" ) );
    run_all_coroutines();
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    // Now give them all exceptions again to make sure it is
    // idempotent.
    exception_p0.set_exception( runtime_error( "test-failed" ) );
    exception_p1.set_exception( runtime_error( "test-failed" ) );
    exception_p2.set_exception( runtime_error( "test-failed" ) );
    run_all_coroutines();
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );
  }

  SECTION( "exception, then cancelled" ) {
    waitable<> w = exception_coro_complex();
    REQUIRE( places == "al" );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_coroutines();
    REQUIRE( places == "almMLbel" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_coroutines();
    REQUIRE( places == "almMLbelmMLfh" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    // This stage will set the exception.
    exception_p1.set_value_emplace();
    run_all_coroutines();
    REQUIRE( w.has_exception() );
    REQUIRE( places == "almMLbelmMLfhiIHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( w.has_exception() );

    // Now cancel it, which should have no effect (though it does
    // reset some state), but just make sure it doesn't crash.
    w.cancel();
    REQUIRE( !w.has_exception() );
    REQUIRE( places == "almMLbelmMLfhiIHFEBA" );
    REQUIRE( !w.ready() );
  }

  SECTION( "exception via promise, then cancelled" ) {
    waitable<> w = exception_coro_complex();
    REQUIRE( places == "al" );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_coroutines();
    REQUIRE( places == "almMLbel" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_coroutines();
    REQUIRE( places == "almMLbelmMLfh" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p1.set_exception( runtime_error( "test-failed" ) );
    run_all_coroutines();
    REQUIRE( w.has_exception() );
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( w.has_exception() );

    run_all_coroutines();
    REQUIRE( w.has_exception() );
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( w.has_exception() );

    // Now cancel it, which should have no effect (though it does
    // reset some state), but just make sure it doesn't crash.
    w.cancel();
    REQUIRE( !w.has_exception() );
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
  }
}

} // namespace
} // namespace rn
