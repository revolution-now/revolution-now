/****************************************************************
**ext-base.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-16.
*
* Description: Unit tests for the src/luapp/ext-base.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/ext-base.hpp"

// Testing
#include "test/luapp/common.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;

LUA_TEST_CASE( "[ext-base] maybe push" ) {
  maybe<int> m;

  SECTION( "nothing" ) {
    push( L, m );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::nil );
    C.pop();
  }

  SECTION( "something" ) {
    m = 5;
    push( L, m );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::number );
    REQUIRE( C.get<int>( -1 ) == 5 );
    C.pop();
  }
}

LUA_TEST_CASE( "[ext-base] maybe get" ) {
  SECTION( "nil" ) {
    push( L, nil );
    maybe<maybe<int>> m = get<maybe<int>>( L, -1 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == nothing );
    C.pop();
  }

  SECTION( "something" ) {
    push( L, 9 );
    maybe<maybe<int>> m = get<maybe<int>>( L, -1 );
    REQUIRE( m.has_value() );
    REQUIRE( m->has_value() );
    REQUIRE( **m == 9 );
    C.pop();
  }
}

} // namespace
} // namespace lua
