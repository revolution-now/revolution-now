/****************************************************************
**indexer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-10.
*
* Description: Unit tests for the src/luapp/indexer.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/indexer.hpp"

// luapp
#include "src/luapp/c-api.hpp"

// Must be last.
#include "test/catch-common.hpp"

// FMT_TO_CATCH( ::rn::UnitId );

namespace lua {
namespace {

using namespace std;

struct MockTable {
  int* pushed = nullptr;

  void create( lua_State* L ) const {
    c_api C = c_api::view( L );
    CHECK( C.dostring( R"(
      return {
        [5] = {
          [1] = {
            hello = "payload"
          }
        }
      }
    )" ) );
  }
};

void push( lua_State* L, MockTable const& mt ) {
  mt.create( L );
  ++( *mt.pushed );
}

TEST_CASE( "[indexer] construct" ) {
  indexer idxr1( 5, MockTable{} );
  indexer idxr2( "5", MockTable{} );
  int     n = 2;
  indexer idxr3( n, MockTable{} );
}

TEST_CASE( "[indexer] index" ) {
  auto idxr = indexer(
      5, MockTable{} )[1]["hello"]['c'][string( "hello" )];
  using expected_t = indexer<
      string,
      indexer<char,
              indexer<char const( & )[6],
                      indexer<int, indexer<int, MockTable>>>>>;
  static_assert( is_same_v<decltype( idxr ), expected_t> );
}

TEST_CASE( "[indexer] push" ) {
  int  n = 0;
  auto proxy =
      indexer( 5, MockTable{ .pushed = &n } )[1]["hello"];

  c_api C;
  REQUIRE( n == 0 );
  REQUIRE( C.stack_size() == 0 );

  push( C.state(), proxy );
  REQUIRE( n == 1 );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.get<string>( -1 ) == "payload" );

  push( C.state(), proxy );
  REQUIRE( n == 2 );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.get<string>( -1 ) == "payload" );

  C.pop( 2 );
  REQUIRE( C.stack_size() == 0 );
}

} // namespace
} // namespace lua
