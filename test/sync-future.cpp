/****************************************************************
**sync-future.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-08.
*
* Description: Unit tests for the sync-future module.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "sync-future-coro.hpp"
#include "sync-future.hpp"

// Must be last.
#include "catch-common.hpp"

// C++ standard library
#include <queue>

FMT_TO_CATCH_T( ( T ), ::rn::sync_future );
FMT_TO_CATCH_T( ( T ), ::rn::sync_promise );

namespace rn {
namespace {

using namespace std;
using namespace rn;

// Unit test for sync_future's default template parameter.
static_assert(
    is_same_v<sync_future<>, sync_future<monostate>> );

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

  void add_callback( std::function<NotifyFunc> ) override {
    SHOULD_NOT_BE_HERE;
  }

  maybe<int> maybe_int;
};

TEST_CASE( "[sync-future] future default construction" ) {
  sync_future<> s_future;
  REQUIRE( s_future.empty() );
  REQUIRE( !s_future.waiting() );
  REQUIRE( !s_future.ready() );
  REQUIRE( !s_future.taken() );
}

TEST_CASE( "[sync-future] future api basic" ) {
  auto ss = make_shared<shared_int_state>();

  sync_future<int> s_future( ss );

  REQUIRE( !s_future.empty() );
  REQUIRE( s_future.waiting() );
  REQUIRE( !s_future.ready() );
  REQUIRE( !s_future.taken() );

  ss->maybe_int = 3;
  REQUIRE( !s_future.empty() );
  REQUIRE( !s_future.waiting() );
  REQUIRE( s_future.ready() );
  REQUIRE( !s_future.taken() );

  REQUIRE( s_future.get() == 3 );
  REQUIRE( !s_future.empty() );
  REQUIRE( !s_future.waiting() );
  REQUIRE( s_future.ready() );
  REQUIRE( s_future.taken() );

  REQUIRE( s_future.get_and_reset() == 3 );
  REQUIRE( s_future.empty() );
  REQUIRE( !s_future.waiting() );
  REQUIRE( !s_future.ready() );
  REQUIRE( s_future.taken() );
}

TEST_CASE( "[sync-future] future api with continuation" ) {
  auto ss = make_shared<shared_int_state>();

  sync_future<int> s_future( ss );

  REQUIRE( !s_future.empty() );
  REQUIRE( s_future.waiting() );
  REQUIRE( !s_future.ready() );
  REQUIRE( !s_future.taken() );

  auto s_future2 =
      s_future.fmap( []( int n ) { return n + 1; } );

  REQUIRE( !s_future.empty() );
  REQUIRE( s_future.waiting() );
  REQUIRE( !s_future.ready() );
  REQUIRE( !s_future.taken() );

  REQUIRE( !s_future2.empty() );
  REQUIRE( s_future2.waiting() );
  REQUIRE( !s_future2.ready() );
  REQUIRE( !s_future2.taken() );

  auto s_future3 = s_future2.fmap(
      []( int n ) { return std::to_string( n ); } );

  REQUIRE( !s_future2.empty() );
  REQUIRE( s_future2.waiting() );
  REQUIRE( !s_future2.ready() );
  REQUIRE( !s_future2.taken() );

  REQUIRE( !s_future3.empty() );
  REQUIRE( s_future3.waiting() );
  REQUIRE( !s_future3.ready() );
  REQUIRE( !s_future3.taken() );

  ss->maybe_int = 3;
  REQUIRE( !s_future.empty() );
  REQUIRE( !s_future2.empty() );
  REQUIRE( !s_future3.empty() );
  REQUIRE( !s_future.waiting() );
  REQUIRE( !s_future2.waiting() );
  REQUIRE( !s_future3.waiting() );
  REQUIRE( s_future.ready() );
  REQUIRE( s_future2.ready() );
  REQUIRE( s_future3.ready() );
  REQUIRE( !s_future.taken() );
  REQUIRE( !s_future2.taken() );
  REQUIRE( !s_future3.taken() );

  REQUIRE( s_future3.get() == "4" );
  REQUIRE( s_future3.taken() );
  auto res3 = s_future3.get_and_reset();
  REQUIRE( s_future3.taken() );
  static_assert( std::is_same_v<decltype( res3 ), std::string> );
  REQUIRE( res3 == "4" );
  REQUIRE( s_future2.get() == 4 );
  REQUIRE( s_future2.taken() );
  auto res2 = s_future2.get_and_reset();
  REQUIRE( s_future2.taken() );
  static_assert( std::is_same_v<decltype( res2 ), int> );
  REQUIRE( res2 == 4 );
  REQUIRE( s_future.get() == 3 );
  REQUIRE( s_future.taken() );
  auto res1 = s_future.get_and_reset();
  REQUIRE( s_future.taken() );
  static_assert( std::is_same_v<decltype( res1 ), int> );
  REQUIRE( res1 == 3 );

  REQUIRE( s_future.empty() );
  REQUIRE( !s_future.waiting() );
  REQUIRE( !s_future.ready() );
  REQUIRE( s_future.taken() );
  REQUIRE( s_future2.empty() );
  REQUIRE( !s_future2.waiting() );
  REQUIRE( !s_future2.ready() );
  REQUIRE( s_future2.taken() );
  REQUIRE( s_future3.empty() );
  REQUIRE( !s_future3.waiting() );
  REQUIRE( !s_future3.ready() );
  REQUIRE( s_future3.taken() );
}

TEST_CASE( "[sync-future] consume" ) {
  bool run = false;

  auto s_future = make_sync_future<int>( 5 ).consume(
      [&]( int ) { run = true; } );

  static_assert(
      std::is_same_v<decltype( s_future ), sync_future<>> );

  REQUIRE( run == false );
  s_future.get_and_reset();
  REQUIRE( run == true );
}

TEST_CASE( "[sync-future] promise api basic api" ) {
  sync_promise<int> s_promise;
  REQUIRE( !s_promise.has_value() );

  sync_future<int> s_future = s_promise.get_future();
  REQUIRE( !s_future.empty() );
  REQUIRE( s_future.waiting() );
  REQUIRE( !s_future.ready() );
  REQUIRE( !s_future.taken() );

  s_promise.set_value( 3 );
  REQUIRE( s_promise.has_value() );
  REQUIRE( !s_future.empty() );
  REQUIRE( !s_future.waiting() );
  REQUIRE( s_future.ready() );
  REQUIRE( !s_future.taken() );

  REQUIRE( s_future.get() == 3 );
  REQUIRE( !s_future.empty() );
  REQUIRE( !s_future.waiting() );
  REQUIRE( s_future.ready() );
  REQUIRE( s_future.taken() );

  REQUIRE( s_future.get_and_reset() == 3 );
  REQUIRE( s_future.empty() );
  REQUIRE( !s_future.waiting() );
  REQUIRE( !s_future.ready() );
  REQUIRE( s_future.taken() );
}

TEST_CASE( "[sync-future] stored" ) {
  sync_promise<int> s_promise;
  REQUIRE( !s_promise.has_value() );

  int  result   = 0;
  auto s_future = s_promise.get_future().stored( &result );
  static_assert( std::is_same_v<decltype( s_future ),
                                sync_future<monostate>> );
  REQUIRE( !s_future.empty() );
  REQUIRE( s_future.waiting() );
  REQUIRE( !s_future.ready() );
  REQUIRE( !s_future.taken() );

  s_promise.set_value( 3 );
  REQUIRE( !s_future.empty() );
  REQUIRE( !s_future.waiting() );
  REQUIRE( s_future.ready() );
  REQUIRE( !s_future.taken() );

  REQUIRE( result == 0 );

  REQUIRE( s_future.get_and_reset() == monostate{} );
  REQUIRE( s_future.empty() );
  REQUIRE( !s_future.waiting() );
  REQUIRE( !s_future.ready() );
  REQUIRE( s_future.taken() );

  REQUIRE( result == 3 );
}

TEST_CASE( "[sync-future] formatting" ) {
  sync_promise<int> s_promise;
  REQUIRE( fmt::format( "{}", s_promise ) == "<empty>" );

  sync_future<int> s_future = s_promise.get_future();
  REQUIRE( fmt::format( "{}", s_future ) == "<waiting>" );

  s_promise.set_value( 3 );
  REQUIRE( fmt::format( "{}", s_promise ) == "<ready>" );
  REQUIRE( fmt::format( "{}", s_future ) == "<ready>" );

  REQUIRE( s_future.get() == 3 );
  REQUIRE( fmt::format( "{}", s_promise ) == "<ready>" );
  REQUIRE( fmt::format( "{}", s_future ) == "<taken>" );

  REQUIRE( s_future.get_and_reset() == 3 );
  REQUIRE( fmt::format( "{}", s_promise ) == "<ready>" );
  REQUIRE( fmt::format( "{}", s_future ) == "<empty>" );
}

TEST_CASE( "[sync-future] bind testing statuses" ) {
  sync_promise<int> s_promise_1;

  auto s_future_1 = s_promise_1.get_future();
  static_assert(
      is_same_v<decltype( s_future_1 ), sync_future<int>> );

  maybe<sync_promise<string>> s_promise_2;
  maybe<sync_promise<string>> s_promise_3;
  auto s_future_2 = s_future_1.bind( [&]( int n ) {
    if( n > 3 ) {
      s_promise_2.emplace();
      return s_promise_2->get_future();
    } else {
      s_promise_3.emplace();
      return s_promise_3->get_future();
    }
  } );
  static_assert(
      is_same_v<decltype( s_future_2 ), sync_future<string>> );

  auto s_future_3 = s_future_2 >> []( string const& s ) {
    if( s.size() > 1 )
      return make_sync_future<int>( 100 );
    else
      return make_sync_future<int>( -100 );
  };
  static_assert(
      is_same_v<decltype( s_future_3 ), sync_future<int>> );

  SECTION( "path 1" ) {
    REQUIRE( !s_future_1.empty() );
    REQUIRE( s_future_1.waiting() );
    REQUIRE( !s_future_1.ready() );
    REQUIRE( !s_future_1.taken() );

    REQUIRE( !s_future_2.empty() );
    REQUIRE( s_future_2.waiting() );
    REQUIRE( !s_future_2.ready() );
    REQUIRE( !s_future_2.taken() );

    REQUIRE( !s_future_3.empty() );
    REQUIRE( s_future_3.waiting() );
    REQUIRE( !s_future_3.ready() );
    REQUIRE( !s_future_3.taken() );

    REQUIRE( !s_promise_2.has_value() );
    REQUIRE( !s_promise_3.has_value() );

    s_promise_1.set_value( 5 );

    REQUIRE( !s_future_1.empty() );
    REQUIRE( !s_future_1.waiting() );
    REQUIRE( s_future_1.ready() );
    REQUIRE( !s_future_1.taken() );

    REQUIRE( !s_future_2.empty() );
    REQUIRE( s_future_2.waiting() );
    REQUIRE( !s_future_2.ready() );
    REQUIRE( !s_future_2.taken() );

    REQUIRE( !s_future_3.empty() );
    REQUIRE( s_future_3.waiting() );
    REQUIRE( !s_future_3.ready() );
    REQUIRE( !s_future_3.taken() );

    REQUIRE( s_promise_2.has_value() );
    REQUIRE( !s_promise_3.has_value() );

    s_promise_2->set_value( "z" );

    REQUIRE( !s_future_1.empty() );
    REQUIRE( !s_future_1.waiting() );
    REQUIRE( s_future_1.ready() );
    REQUIRE( !s_future_1.taken() );

    REQUIRE( !s_future_2.empty() );
    REQUIRE( !s_future_2.waiting() );
    REQUIRE( s_future_2.ready() );
    REQUIRE( !s_future_2.taken() );

    REQUIRE( !s_future_3.empty() );
    REQUIRE( !s_future_3.waiting() );
    REQUIRE( s_future_3.ready() );
    REQUIRE( !s_future_3.taken() );

    REQUIRE( s_future_1.get() == 5 );
    REQUIRE( s_future_1.taken() );
    REQUIRE( !s_future_2.taken() );
    REQUIRE( !s_future_3.taken() );

    REQUIRE( s_future_2.get() == "z" );
    REQUIRE( s_future_1.taken() );
    REQUIRE( s_future_2.taken() );
    REQUIRE( !s_future_3.taken() );

    REQUIRE( s_future_3.get() == -100 );
    REQUIRE( s_future_1.taken() );
    REQUIRE( s_future_2.taken() );
    REQUIRE( s_future_3.taken() );

    REQUIRE( !s_future_1.empty() );
    REQUIRE( !s_future_1.waiting() );
    REQUIRE( s_future_1.ready() );

    REQUIRE( !s_future_2.empty() );
    REQUIRE( !s_future_2.waiting() );
    REQUIRE( s_future_2.ready() );

    REQUIRE( !s_future_3.empty() );
    REQUIRE( !s_future_3.waiting() );
    REQUIRE( s_future_3.ready() );
  }

  SECTION( "path 2" ) {
    REQUIRE( !s_future_1.empty() );
    REQUIRE( s_future_1.waiting() );
    REQUIRE( !s_future_1.ready() );
    REQUIRE( !s_future_1.taken() );

    REQUIRE( !s_future_2.empty() );
    REQUIRE( s_future_2.waiting() );
    REQUIRE( !s_future_2.ready() );
    REQUIRE( !s_future_2.taken() );

    REQUIRE( !s_future_3.empty() );
    REQUIRE( s_future_3.waiting() );
    REQUIRE( !s_future_3.ready() );
    REQUIRE( !s_future_3.taken() );

    REQUIRE( !s_promise_2.has_value() );
    REQUIRE( !s_promise_3.has_value() );

    s_promise_1.set_value( 2 );

    REQUIRE( !s_future_1.empty() );
    REQUIRE( !s_future_1.waiting() );
    REQUIRE( s_future_1.ready() );
    REQUIRE( !s_future_1.taken() );

    REQUIRE( !s_future_2.empty() );
    REQUIRE( s_future_2.waiting() );
    REQUIRE( !s_future_2.ready() );
    REQUIRE( !s_future_2.taken() );

    REQUIRE( !s_future_3.empty() );
    REQUIRE( s_future_3.waiting() );
    REQUIRE( !s_future_3.ready() );
    REQUIRE( !s_future_3.taken() );

    REQUIRE( !s_promise_2.has_value() );
    REQUIRE( s_promise_3.has_value() );

    s_promise_3->set_value( "zz" );

    REQUIRE( !s_future_1.empty() );
    REQUIRE( !s_future_1.waiting() );
    REQUIRE( s_future_1.ready() );
    REQUIRE( !s_future_1.taken() );

    REQUIRE( !s_future_2.empty() );
    REQUIRE( !s_future_2.waiting() );
    REQUIRE( s_future_2.ready() );
    REQUIRE( !s_future_2.taken() );

    REQUIRE( !s_future_3.empty() );
    REQUIRE( !s_future_3.waiting() );
    REQUIRE( s_future_3.ready() );
    REQUIRE( !s_future_3.taken() );

    REQUIRE( s_future_1.get() == 2 );
    REQUIRE( s_future_1.taken() );
    REQUIRE( !s_future_2.taken() );
    REQUIRE( !s_future_3.taken() );

    REQUIRE( s_future_2.get() == "zz" );
    REQUIRE( s_future_1.taken() );
    REQUIRE( s_future_2.taken() );
    REQUIRE( !s_future_3.taken() );

    REQUIRE( s_future_3.get() == 100 );
    REQUIRE( s_future_1.taken() );
    REQUIRE( s_future_2.taken() );
    REQUIRE( s_future_3.taken() );

    REQUIRE( !s_future_1.empty() );
    REQUIRE( !s_future_1.waiting() );
    REQUIRE( s_future_1.ready() );

    REQUIRE( !s_future_2.empty() );
    REQUIRE( !s_future_2.waiting() );
    REQUIRE( s_future_2.ready() );

    REQUIRE( !s_future_3.empty() );
    REQUIRE( !s_future_3.waiting() );
    REQUIRE( s_future_3.ready() );
  }
}

TEST_CASE( "[sync-future] bind testing results" ) {
  sync_promise<int>           s_promise_1;
  maybe<sync_promise<string>> s_promise_2;
  maybe<sync_promise<string>> s_promise_3;

  auto s_future = s_promise_1.get_future() >> [&]( int n ) {
    if( n > 3 ) {
      s_promise_2.emplace();
      return s_promise_2->get_future();
    } else {
      s_promise_3.emplace();
      return s_promise_3->get_future();
    }
  } >> []( string const& s ) {
    if( s.size() > 1 )
      return make_sync_future<int>( 100 );
    else
      return make_sync_future<int>( -100 );
  } >> []( int m ) {
    return make_sync_future<int>( m * 9 );
  } >> []( int m ) {
    return make_sync_future<string>( to_string( m ) );
  };

  SECTION( "path 1" ) {
    REQUIRE( !s_future.ready() );
    s_promise_1.set_value( 5 );
    REQUIRE( !s_future.ready() );
    s_promise_2->set_value( "z" );
    REQUIRE( s_future.ready() );
    REQUIRE( s_future.get() == "-900" );
  }

  SECTION( "path 2" ) {
    REQUIRE( !s_future.ready() );
    s_promise_1.set_value( 2 );
    REQUIRE( !s_future.ready() );
    s_promise_3->set_value( "zz" );
    REQUIRE( s_future.ready() );
    REQUIRE( s_future.get() == "900" );
  }

  SECTION( "path 3" ) {
    REQUIRE( !s_future.ready() );
    s_promise_1.set_value( 2 );
    REQUIRE( !s_future.ready() );
    s_promise_3->set_value( "z" );
    REQUIRE( s_future.ready() );
    REQUIRE( s_future.get() == "-900" );
  }
}

template<typename T>
using sf_coro_promise = typename coro::coroutine_traits<
    sync_future<T>>::promise_type;

queue<variant<sf_coro_promise<int>, sf_coro_promise<double>>>
    g_promises;

void deliver_promise() {
  struct Setter {
    void operator()( sf_coro_promise<int>& p ) { p.set( 1 ); }
    void operator()( sf_coro_promise<double>& p ) {
      p.set( 2.2 );
    }
  };
  if( !g_promises.empty() ) {
    visit( Setter{}, g_promises.front() );
    g_promises.pop();
  }
}

sync_future<int> sync_future_int() {
  sf_coro_promise<int> p;
  g_promises.emplace( p );
  return p.get_future();
}

sync_future<double> sync_future_double() {
  sf_coro_promise<double> p;
  g_promises.emplace( p );
  return p.get_future();
}

sync_future<int> sync_future_sum() {
  co_return                        //
      co_await sync_future_int() + //
      co_await sync_future_int() + //
      co_await sync_future_int();
}

template<typename Func, typename... Args>
auto co_invoke( Func&& func, Args... args )
    -> sync_future<decltype( std::forward<Func>( func )(
        std::declval<typename Args::value_t>()... ) )> {
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

sync_future<string> sync_future_string() {
  int    n = co_await sync_future_sum();
  double d = co_await sync_future_double();

  int m = co_await sync_future_sum();
  for( int i = 0; i < m; ++i ) //
    d += co_await sync_future_double();

  int sum = co_await co_lift{ std::plus<>{} }(
      sync_future_sum(), sync_future_sum() );

  auto f = [&]() -> sync_future<int> {
    int res = co_await sync_future_sum() *
              int( co_await sync_future_double() );
    co_return res;
  };
  int z = co_await f() + sum;

  co_return to_string( n ) + "-" + to_string( z ) + "-" +
      to_string( d );
}

TEST_CASE( "[sync-future] coro" ) {
  sync_future<string> sfs = sync_future_string();
  int                 i   = 0;
  while( !sfs.ready() ) {
    ++i;
    deliver_promise();
  }
  REQUIRE( sfs.get() == "3-12-8.800000" );
  REQUIRE( i == 20 );
}

} // namespace
} // namespace rn
