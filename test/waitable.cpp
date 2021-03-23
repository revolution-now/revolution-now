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

// Unit test for waitable's default template parameter.
static_assert( is_same_v<waitable<>, waitable<monostate>> );

struct shared_int_state
  : public internal::sync_shared_state_base<int> {
  using Base_t = internal::sync_shared_state_base<int>;
  using typename Base_t::NotifyFunc;
  ~shared_int_state() override = default;

  bool has_value() const override {
    return maybe_int.has_value();
  }

  int get() const override {
    CHECK( has_value() );
    return *maybe_int;
  }

  void add_copyable_callback(
      std::function<NotifyFunc> ) override {
    SHOULD_NOT_BE_HERE;
  }

  void add_movable_callback(
      base::unique_func<NotifyFunc> ) override {
    SHOULD_NOT_BE_HERE;
  }

  void clear_callbacks() override {}

  maybe<int> maybe_int;
};

TEST_CASE( "[waitable] future default construction" ) {
  waitable<> s_future;
  REQUIRE( s_future.ready() );
}

TEST_CASE( "[waitable] future api basic" ) {
  auto ss = make_shared<shared_int_state>();

  waitable<int> s_future( ss );

  REQUIRE( !s_future.ready() );

  ss->maybe_int = 3;
  REQUIRE( s_future.ready() );
  REQUIRE( s_future.get() == 3 );
}

TEST_CASE( "[waitable] future api with continuation" ) {
  auto ss = make_shared<shared_int_state>();

  waitable<int> s_future( ss );

  REQUIRE( !s_future.ready() );

  auto s_future2 =
      s_future.fmap( []( int n ) { return n + 1; } );

  REQUIRE( !s_future.ready() );

  REQUIRE( !s_future2.ready() );

  auto s_future3 = s_future2.fmap(
      []( int n ) { return std::to_string( n ); } );

  REQUIRE( !s_future2.ready() );

  REQUIRE( !s_future3.ready() );

  ss->maybe_int = 3;
  REQUIRE( s_future.ready() );
  REQUIRE( s_future2.ready() );
  REQUIRE( s_future3.ready() );

  REQUIRE( s_future3.get() == "4" );
  auto res3 = s_future3.get();
  static_assert( std::is_same_v<decltype( res3 ), std::string> );
  REQUIRE( res3 == "4" );
  REQUIRE( s_future2.get() == 4 );
  auto res2 = s_future2.get();
  static_assert( std::is_same_v<decltype( res2 ), int> );
  REQUIRE( res2 == 4 );
  REQUIRE( s_future.get() == 3 );
  auto res1 = s_future.get();
  static_assert( std::is_same_v<decltype( res1 ), int> );
  REQUIRE( res1 == 3 );

  REQUIRE( s_future.ready() );
  REQUIRE( s_future2.ready() );
  REQUIRE( s_future3.ready() );
}

TEST_CASE( "[waitable] consume" ) {
  bool run = false;

  auto s_future = make_waitable<int>( 5 ).consume(
      [&]( int ) { run = true; } );

  static_assert(
      std::is_same_v<decltype( s_future ), waitable<>> );

  REQUIRE( run == false );
  s_future.get();
  REQUIRE( run == true );
}

TEST_CASE( "[waitable] promise api basic api" ) {
  waitable_promise<int> s_promise;
  REQUIRE( !s_promise.has_value() );

  waitable<int> s_future = s_promise.get_waitable();
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

  waitable<int> s_future = s_promise.get_waitable();
  REQUIRE( fmt::format( "{}", s_future ) == "<waiting>" );

  s_promise.set_value_if_not_set( 3 );
  REQUIRE( fmt::format( "{}", s_promise ) == "<ready>" );
  REQUIRE( fmt::format( "{}", s_future ) == "<ready>" );

  REQUIRE( s_future.get() == 3 );
  REQUIRE( fmt::format( "{}", s_promise ) == "<ready>" );
}

template<typename T>
using w_coro_promise =
    typename coro::coroutine_traits<waitable<T>>::promise_type;

queue<variant<w_coro_promise<int>, w_coro_promise<double>>>
    g_promises;

void deliver_promise() {
  struct Setter {
    void operator()( w_coro_promise<int>& p ) { p.set( 1 ); }
    void operator()( w_coro_promise<double>& p ) {
      p.set( 2.2 );
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

struct LogDestruction {
  LogDestruction( bool& b_ ) : b( b_ ) {}
  ~LogDestruction() { b = true; }
  bool& b;
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
  waitable<>         w  = p.get_waitable();
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
  waitable<>         w  = p.get_waitable();
  w.shared_state()->add_callback( std::move( callback ) );
  CHECK( !callbacks_released );
  p.set_value_emplace();
  CHECK( callbacks_released );
  p2 = {};
  CHECK( callbacks_released );
  p3 = {};
  CHECK( callbacks_released );
}

} // namespace
} // namespace rn
