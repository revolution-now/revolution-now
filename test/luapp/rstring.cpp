/****************************************************************
**rstring.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: Unit tests for the src/luapp/string.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/rstring.hpp"

// Testing
#include "test/luapp/common.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

/****************************************************************
** rstring objects
*****************************************************************/
LUA_TEST_CASE( "[rstring] string equality" ) {
  boolean       b   = true;
  integer       i   = 1;
  floating      f   = 2.3;
  lightuserdata lud = C.newuserdata( 10 );
  C.pop();

  C.push( "hello" );
  rstring o1( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  C.push( "hello" );
  rstring o2( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  C.push( "world" );
  rstring o3( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  rstring o4 = o1;

  REQUIRE( o1 == o2 );
  REQUIRE( o1 != o3 );
  REQUIRE( o2 != o3 );
  REQUIRE( o1 == o4 );
  REQUIRE( o2 == o4 );
  REQUIRE( o4 != o3 );

  REQUIRE( o1 != nil );
  REQUIRE( o1 != b );
  REQUIRE( o1 != i );
  REQUIRE( o1 != f );
  REQUIRE( o1 != lud );
  REQUIRE( nil != o1 );
  REQUIRE( b != o1 );
  REQUIRE( i != o1 );
  REQUIRE( f != o1 );
  REQUIRE( lud != o1 );
}

LUA_TEST_CASE( "[rstring] as_cpp / string literal cmp" ) {
  C.push( "hello" );
  int ref = C.ref_registry();
  REQUIRE( C.stack_size() == 0 );
  rstring s( C.this_cthread(), ref );

  REQUIRE( s == "hello" );
  REQUIRE( "hello" == s );
  REQUIRE( s != "world" );
  REQUIRE( "world" != s );

  REQUIRE( s.as_cpp() == "hello" );
}

} // namespace
} // namespace lua
