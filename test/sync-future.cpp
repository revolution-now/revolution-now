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
#include "sync-future.hpp"

// Must be last.
#include "catch-common.hpp"

FMT_TO_CATCH_T( ( T ), ::rn::sync_future );
FMT_TO_CATCH_T( ( T ), ::rn::sync_promise );

namespace {

using namespace std;
using namespace rn;

// Unit test for sync_future's default template parameter.
static_assert(
    is_same_v<rn::sync_future<>, rn::sync_future<monostate>> );

struct shared_int_state
  : public internal::sync_shared_state_base<int> {
  ~shared_int_state() override = default;

  bool has_value() const override {
    return maybe_int.has_value();
  }

  int get() const override {
    CHECK( has_value() );
    return *maybe_int;
  }

  Opt<int> maybe_int;
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
      s_future.then( []( int n ) { return n + 1; } );

  REQUIRE( !s_future.empty() );
  REQUIRE( s_future.waiting() );
  REQUIRE( !s_future.ready() );
  REQUIRE( !s_future.taken() );

  REQUIRE( !s_future2.empty() );
  REQUIRE( s_future2.waiting() );
  REQUIRE( !s_future2.ready() );
  REQUIRE( !s_future2.taken() );

  auto s_future3 = s_future2.then(
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

  auto s_future = rn::make_sync_future<int>( 5 ).consume(
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
  REQUIRE_THROWS_AS_RN( s_promise.set_value( 4 ) );
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
                                ::rn::sync_future<monostate>> );
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

} // namespace
