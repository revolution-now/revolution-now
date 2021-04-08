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
  auto ss = make_shared<detail::sync_shared_state<int>>();

  waitable<int> s_future( ss );

  REQUIRE( !s_future.ready() );

  ss->maybe_value = 3;
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

// Test that when the number of promises that refer to a shared
// state go to zero that the callbacks get released.
TEST_CASE( "[waitable] promise ref count" ) {
  bool callbacks_released = false;
  auto callback = [_ = LogDestruction( callbacks_released )](
                      monostate const& ) {};
  waitable_promise<> p;
  auto               p2 = p;
  auto               p3 = p;
  waitable<>         w  = p.waitable();
  w.shared_state()->add_callback( std::move( callback ) );
  CHECK( !callbacks_released );
  p = {};
  CHECK( !callbacks_released );
  p2 = {};
  CHECK( !callbacks_released );
  p3 = {};
  CHECK( callbacks_released );
}

// Test that the callbacks get released as soon as a promise is
// fulfilled.
TEST_CASE( "[waitable] set value clears callbacks" ) {
  bool callbacks_released = false;
  auto callback = [_ = LogDestruction( callbacks_released )](
                      monostate const& ) {};
  waitable_promise<> p;
  auto               p2 = p;
  auto               p3 = p;
  waitable<>         w  = p.waitable();
  w.shared_state()->add_callback( std::move( callback ) );
  CHECK( !callbacks_released );
  p.set_value_emplace();
  CHECK( callbacks_released );
  p2 = {};
  CHECK( callbacks_released );
  p3 = {};
  CHECK( callbacks_released );
}

/****************************************************************
** Coroutines
*****************************************************************/
template<typename T>
using w_coro_promise =
    typename coro::coroutine_traits<waitable<T>>::promise_type;

queue<variant<w_coro_promise<int>, w_coro_promise<double>>>
    g_promises;

void deliver_promise() {
  struct Setter {
    void operator()( w_coro_promise<int>& p ) {
      p.return_value( 1 );
    }
    void operator()( w_coro_promise<double>& p ) {
      p.return_value( 2.2 );
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
  return p.get_return_object();
}

waitable<double> waitable_double() {
  w_coro_promise<double> p;
  g_promises.emplace( p );
  return p.get_return_object();
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
  co_return std::forward<Func>( func )( ( co_await args )... );
}

template<typename Func>
struct co_lift {
  Func func_;
  co_lift( Func&& func ) : func_( std::move( func ) ) {}
  co_lift( Func const& func ) : func_( func ) {}
  template<typename... Args>
  auto operator()( Args... args ) {
    return co_invoke( func_, args... );
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

} // namespace
} // namespace rn
