/****************************************************************
**wait.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-08.
*
* Description: Unit tests for the wait module.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "co-scheduler.hpp"
#include "co-wait.hpp"
#include "wait.hpp"

// base
#include "base/scope-exit.hpp"

// Must be last.
#include "catch-common.hpp"

// C++ standard library
#include <queue>

FMT_TO_CATCH_T( ( T ), ::rn::wait );
FMT_TO_CATCH_T( ( T ), ::rn::wait_promise );

namespace rn {
namespace {

using namespace std;
using namespace rn;

using ::Catch::Equals;

// Unit test for wait's default template parameter.
static_assert( is_same_v<wait<>, wait<monostate>> );

TEST_CASE( "[wait] future api basic" ) {
  auto ss = make_shared<detail::wait_shared_state<int>>();

  wait<int> s_future( ss );

  REQUIRE( !s_future.ready() );

  ss->set( 3 );
  REQUIRE( s_future.ready() );
  REQUIRE( s_future.get() == 3 );
}

TEST_CASE( "[wait] promise api basic api" ) {
  wait_promise<int> s_promise;
  REQUIRE( !s_promise.has_value() );

  wait<int> s_future = s_promise.wait();
  REQUIRE( !s_future.ready() );

  s_promise.set_value( 3 );
  s_promise.set_value_if_not_set( 4 );
  REQUIRE( s_promise.has_value() );
  REQUIRE( s_future.ready() );
  REQUIRE( s_future.get() == 3 );
}

TEST_CASE( "[wait] formatting" ) {
  wait_promise<int> s_promise;
  REQUIRE( fmt::format( "{}", s_promise ) == "<empty>" );

  wait<int> s_future = s_promise.wait();
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
using w_coro_promise = wait_promise<T>;

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

wait<int> wait_int() {
  w_coro_promise<int> p;
  g_promises.emplace( p );
  return p.wait();
}

wait<double> wait_double() {
  w_coro_promise<double> p;
  g_promises.emplace( p );
  return p.wait();
}

wait<int> wait_sum() {
  co_return                 //
      co_await wait_int() + //
      co_await wait_int() + //
      co_await wait_int();
}

template<typename Func, typename... Args>
auto co_invoke( Func&& func, Args... args )
    -> wait<decltype( std::forward<Func>( func )(
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

wait<string> wait_string() {
  int    n = co_await wait_sum();
  double d = co_await wait_double();

  int m = co_await wait_sum();
  for( int i = 0; i < m; ++i ) //
    d += co_await wait_double();

  int sum = co_await co_lift{ std::plus<>{} }( wait_sum(),
                                               wait_sum() );

  auto f = [&]() -> wait<int> {
    int res =
        co_await wait_sum() * int( co_await wait_double() );
    co_return res;
  };
  int z = co_await f() + sum;

  co_return to_string( n ) + "-" + to_string( z ) + "-" +
      to_string( d );
}

TEST_CASE( "[wait] coro" ) {
  wait<string> ws = wait_string();
  int          i  = 0;
  while( !ws.ready() ) {
    ++i;
    deliver_promise();
    run_all_cpp_coroutines();
  }
  REQUIRE( ws.get() == "3-12-8.800000" );
  REQUIRE( i == 20 );
}

/****************************************************************
** Coroutine Cancellation
*****************************************************************/
#if !defined( CORO_TEST_DISABLE_FOR_GCC )
vector<string> string_log;

struct LogDestructionStr {
  LogDestructionStr( string const& s_ ) : s( s_ ) {}
  ~LogDestructionStr() { string_log.push_back( "~~~: " + s ); }
  string s;
};

void log_str( string const& s ) {
  string_log.push_back( "run: " + s );
}

wait_promise<string> p;
wait_promise<int>    p0;
wait_promise<int>    p1;

wait<string> coro() {
  LogDestructionStr lds( "coro" );
  log_str( "coro" );
  return p.wait();
}

wait<int> coro0() {
  LogDestructionStr lds( "coro0" );
  log_str( "coro0" );
  return p0.wait();
}

wait<int> coro1() {
  LogDestructionStr lds( "coro1" );
  log_str( "coro1" );
  return p1.wait();
}

wait<string> coro2() {
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

wait<string> coro3() {
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

TEST_CASE( "[wait] coro cancel" ) {
  p  = {};
  p0 = {};
  p1 = {};
  string_log.clear();

  wait<string> ws = coro3();

  SECTION( "no cancel" ) {
    REQUIRE( !ws.ready() );
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p0.set_value( 5 );
    REQUIRE( number_of_queued_cpp_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_cpp_coroutines();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p1.set_value( 7 );
    REQUIRE( number_of_queued_cpp_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_cpp_coroutines();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p.set_value( "!" );
    REQUIRE( number_of_queued_cpp_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_cpp_coroutines();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( ws.ready() );
    // Cancelling a ready wait should do nothing.
    ws.cancel();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
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
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    ws.cancel();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
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
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p0.set_value( 5 );
    REQUIRE( number_of_queued_cpp_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    ws.cancel();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
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
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p0.set_value( 5 );
    REQUIRE( number_of_queued_cpp_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_cpp_coroutines();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    ws.cancel();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
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
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p0.set_value( 5 );
    REQUIRE( number_of_queued_cpp_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_cpp_coroutines();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p1.set_value( 7 );
    REQUIRE( number_of_queued_cpp_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    ws.cancel();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
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
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p0.set_value( 5 );
    REQUIRE( number_of_queued_cpp_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_cpp_coroutines();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p1.set_value( 7 );
    REQUIRE( number_of_queued_cpp_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_cpp_coroutines();
    ws.cancel();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
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
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p0.set_value( 5 );
    REQUIRE( number_of_queued_cpp_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_cpp_coroutines();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    REQUIRE( !ws.ready() );
    p1.set_value( 7 );
    REQUIRE( number_of_queued_cpp_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    run_all_cpp_coroutines();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
    p.set_value( "!" );
    REQUIRE( number_of_queued_cpp_coroutines() == 1 );
    REQUIRE( !ws.ready() );
    ws.cancel();
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );
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

TEST_CASE( "[wait] coro cancel by wait out-of-scope" ) {
  p  = {};
  p0 = {};
  p1 = {};
  string_log.clear();

  SECTION( "cancel coro with scheduled" ) {
    {
      wait<string> ws = coro3();

      REQUIRE( number_of_queued_cpp_coroutines() == 0 );
      REQUIRE( !ws.ready() );
      p0.set_value( 5 );
      REQUIRE( number_of_queued_cpp_coroutines() == 1 );
      REQUIRE( !ws.ready() );
      run_all_cpp_coroutines();
      REQUIRE( number_of_queued_cpp_coroutines() == 0 );
      REQUIRE( !ws.ready() );
      p1.set_value( 7 );
      REQUIRE( number_of_queued_cpp_coroutines() == 1 );
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
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );

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
      wait<string> ws = coro3();

      REQUIRE( number_of_queued_cpp_coroutines() == 0 );
      REQUIRE( !ws.ready() );
      p0.set_value( 5 );
      REQUIRE( number_of_queued_cpp_coroutines() == 1 );
      REQUIRE( !ws.ready() );
      run_all_cpp_coroutines();
      REQUIRE( number_of_queued_cpp_coroutines() == 0 );
      REQUIRE( !ws.ready() );
      p1.set_value( 7 );
      REQUIRE( number_of_queued_cpp_coroutines() == 1 );
      REQUIRE( !ws.ready() );
      run_all_cpp_coroutines();
      REQUIRE( number_of_queued_cpp_coroutines() == 0 );
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
    REQUIRE( number_of_queued_cpp_coroutines() == 0 );

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

TEST_CASE( "[wait] simple exception" ) {
  wait_promise<> p;
  wait<>         w = p.wait();

  REQUIRE( !w.ready() );
  p.set_exception( runtime_error( "test" ) );
  REQUIRE( !w.ready() );
}

wait<> throws_eagerly_from_non_coro() {
  // This is not a coroutine (even though it returns a wait),
  // and so the following exception will just fly out.
  throw runtime_error( "eager exception" );
}

wait<> doomed_awaiter_on_non_coro() {
  co_await throws_eagerly_from_non_coro();
}

wait<> throws_eagerly_from_coro() {
  // This is a coroutine (because we have a co_return in it),
  // which changes how it will handle the below exception (it
  // will catch it instead of letting it fly).
  throw runtime_error( "eager exception" );
  co_return;
}

wait<> doomed_awaiter_on_coro() {
  co_await throws_eagerly_from_coro();
}

TEST_CASE( "[wait] eager co_await'd exception" ) {
  wait<> w2 = doomed_awaiter_on_non_coro();
  REQUIRE( w2.has_exception() );
  REQUIRE( !w2.ready() );

  wait<> w1 = doomed_awaiter_on_coro();
  REQUIRE( w1.has_exception() );
  REQUIRE( !w1.ready() );
}

wait_promise<> exception_p0;
wait_promise<> exception_p1;
wait_promise<> exception_p2;
string         places;

wait<> exception_coro_early_level_2() {
  places += 'c';
  SCOPE_EXIT( places += 'C' );
  throw runtime_error( "test" );
  places += 'd';
  SCOPE_EXIT( places += 'D' );
}

wait<> exception_coro_early_level_1() {
  places += 'a';
  SCOPE_EXIT( places += 'A' );
  wait<> w = exception_coro_early_level_2();
  REQUIRE( w.has_exception() );
  co_await std::move( w );
  places += 'b';
  SCOPE_EXIT( places += 'B' );
}

TEST_CASE( "[wait] exception coro early two_levels" ) {
  places.clear();

  wait<> w = exception_coro_early_level_1();
  REQUIRE( places == "acCA" );
  REQUIRE( !w.ready() );
  REQUIRE( w.has_exception() );
}

wait<> exception_coro_simple() {
  places += 'a';
  SCOPE_EXIT( places += 'A' );
  co_await exception_p0.wait();
  places += 'b';
  SCOPE_EXIT( places += 'B' );
  throw runtime_error( "test" );
  places += 'c';
  SCOPE_EXIT( places += 'C' );
}

TEST_CASE( "[wait] exception coro simple" ) {
  places.clear();
  exception_p0 = {};

  SECTION( "forward cancellation" ) {
    // We don't really need to test this here because it's cov-
    // ered in other tests, but just do it anyway.
    wait<> w = exception_coro_simple();
    REQUIRE( places == "a" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    w.cancel();
    REQUIRE( places == "aA" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );
  }

  SECTION( "exception (backward cancellation)" ) {
    wait<> w = exception_coro_simple();
    REQUIRE( places == "a" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_cpp_coroutines();
    REQUIRE( places == "abBA" );
    REQUIRE( !w.ready() );
    REQUIRE( w.has_exception() );
  }
}

wait<> exception_0() {
  exception_p0 = {};
  places += 'l';
  SCOPE_EXIT( places += 'L' );
  co_await exception_p0.wait();
  places += 'm';
  SCOPE_EXIT( places += 'M' );
}

wait<> exception_2() {
  places += 'h';
  SCOPE_EXIT( places += 'H' );
  co_await exception_p1.wait();
  places += 'i';
  SCOPE_EXIT( places += 'I' );
  throw runtime_error( "test" );
  places += 'j';
  SCOPE_EXIT( places += 'J' );
  co_await exception_p2.wait();
  places += 'k';
  SCOPE_EXIT( places += 'K' );
}

wait<> exception_1() {
  places += 'e';
  SCOPE_EXIT( places += 'E' );
  co_await exception_0();
  places += 'f';
  SCOPE_EXIT( places += 'F' );
  co_await exception_2();
  places += 'g';
  SCOPE_EXIT( places += 'G' );
}

wait<> exception_coro_complex() {
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

TEST_CASE( "[wait] exception coro complex" ) {
  places.clear();
  exception_p0 = {};
  exception_p1 = {};
  exception_p2 = {};

  SECTION( "cancelled, then exception via promise" ) {
    // We don't really need to test this here because it's cov-
    // ered in other tests, but just do it anyway.
    wait<> w = exception_coro_complex();
    REQUIRE( places == "al" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_cpp_coroutines();
    REQUIRE( places == "almMLbel" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_cpp_coroutines();
    REQUIRE( places == "almMLbelmMLfh" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    w.cancel();
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    run_all_cpp_coroutines();
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    // Since it is already cancelled, an exception should not re-
    // ally do anything, but we just want to make sure that it
    // doesn't crash and doesn't change anything.
    exception_p0.set_exception( runtime_error( "test-failed" ) );
    run_all_cpp_coroutines();
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p1.set_exception( runtime_error( "test-failed" ) );
    run_all_cpp_coroutines();
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p2.set_exception( runtime_error( "test-failed" ) );
    run_all_cpp_coroutines();
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    // Now give them all exceptions again to make sure it is
    // idempotent.
    exception_p0.set_exception( runtime_error( "test-failed" ) );
    exception_p1.set_exception( runtime_error( "test-failed" ) );
    exception_p2.set_exception( runtime_error( "test-failed" ) );
    run_all_cpp_coroutines();
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );
  }

  SECTION( "exception, then cancelled" ) {
    wait<> w = exception_coro_complex();
    REQUIRE( places == "al" );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_cpp_coroutines();
    REQUIRE( places == "almMLbel" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_cpp_coroutines();
    REQUIRE( places == "almMLbelmMLfh" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    // This stage will set the exception.
    exception_p1.set_value_emplace();
    run_all_cpp_coroutines();
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
    wait<> w = exception_coro_complex();
    REQUIRE( places == "al" );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_cpp_coroutines();
    REQUIRE( places == "almMLbel" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p0.set_value_emplace();
    run_all_cpp_coroutines();
    REQUIRE( places == "almMLbelmMLfh" );
    REQUIRE( !w.ready() );
    REQUIRE( !w.has_exception() );

    exception_p1.set_exception( runtime_error( "test-failed" ) );
    run_all_cpp_coroutines();
    REQUIRE( w.has_exception() );
    REQUIRE( places == "almMLbelmMLfhHFEBA" );
    REQUIRE( !w.ready() );
    REQUIRE( w.has_exception() );

    run_all_cpp_coroutines();
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
#endif

} // namespace
} // namespace rn
