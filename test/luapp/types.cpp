/****************************************************************
**types.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-04.
*
* Description: Unit tests for the src/luapp/types.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/types.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/c-api.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::e_lua_type );

namespace lua {
namespace {

using namespace std;

LUA_TEST_CASE( "[types] push" ) {
  push( L, nil );
  REQUIRE( C.type_of( -1 ) == e_lua_type::nil );

  push( L, 5 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::number );
  REQUIRE( C.isinteger( -1 ) );
  REQUIRE( C.get<int>( -1 ) == 5 );

  push( L, 5.0 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::number );
  REQUIRE_FALSE( C.isinteger( -1 ) );
  REQUIRE( C.get<double>( -1 ) == 5.0 );

  push( L, 5.5 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::number );
  REQUIRE_FALSE( C.isinteger( -1 ) );
  REQUIRE( C.get<double>( -1 ) == 5.5 );

  push( L, true );
  REQUIRE( C.type_of( -1 ) == e_lua_type::boolean );
  REQUIRE( C.get<bool>( -1 ) == true );

  push( L, (void*)L );
  REQUIRE( C.type_of( -1 ) == e_lua_type::lightuserdata );
  REQUIRE( C.get<void*>( -1 ) == L );

  push( L, "hello" );
  REQUIRE( C.type_of( -1 ) == e_lua_type::string );
  REQUIRE( C.get<string>( -1 ) == "hello" );
}

LUA_TEST_CASE( "[types] equality" ) {
  SECTION( "nil with nil" ) {
    REQUIRE( nil == nil );
    REQUIRE( nil == nil_t{} );
  }

  SECTION( "nil with boolean" ) {
    REQUIRE( nil != true );
    REQUIRE( nil != false );
    REQUIRE( true != nil );
    REQUIRE( false != nil );
    boolean b1 = true;
    boolean b2 = false;
    REQUIRE( nil != b1 );
    REQUIRE( nil != b2 );
    REQUIRE( b1 != nil );
    REQUIRE( b2 != nil );
  }

  SECTION( "nil with lightuserdata" ) {
    lightuserdata lud1 = nullptr;
    lightuserdata lud2 = C.newuserdata( 10 );
    REQUIRE( nil != lud1 );
    REQUIRE( lud1 != nil );
    REQUIRE( nil != lud2 );
    REQUIRE( lud2 != nil );
  }

  SECTION( "nil with integer" ) {
    REQUIRE( nil != 1 );
    REQUIRE( nil != (int)0 );
    REQUIRE( 1 != nil );
    REQUIRE( (int)0 != nil );
    integer i1 = 1;
    integer i2 = 0;
    REQUIRE( nil != i1 );
    REQUIRE( nil != i2 );
    REQUIRE( i1 != nil );
    REQUIRE( i2 != nil );
  }

  SECTION( "nil with floating" ) {
    REQUIRE( nil != 1. );
    REQUIRE( nil != 0. );
    REQUIRE( 1. != nil );
    REQUIRE( 0. != nil );
    floating f1 = 1.;
    floating f2 = 0.;
    REQUIRE( nil != f1 );
    REQUIRE( nil != f2 );
    REQUIRE( f1 != nil );
    REQUIRE( f2 != nil );
  }

  SECTION( "boolean with boolean" ) {
    boolean b1 = true;
    boolean b2 = false;
    REQUIRE( b1 == b1 );
    REQUIRE( b2 == b2 );
    REQUIRE( b1 != b2 );
    REQUIRE( b2 != b1 );
    REQUIRE( b1 == true );
    REQUIRE( b1 != false );
    REQUIRE( true == b1 );
    REQUIRE( false != b1 );
  }

  SECTION( "boolean with lightuserdata" ) {
    lightuserdata lud = C.newuserdata( 10 );
    boolean       b   = true;
    REQUIRE( lud != b );
    REQUIRE( b != lud );
    REQUIRE( lud != true );
    REQUIRE( true != lud );
  }

  // This section has some extra parens around the comparisons to
  // prevent Catch's expression templates from messing up our
  // comparison overload selection.
  SECTION( "boolean with integer" ) {
    boolean b = false;
    integer i = 0;
    REQUIRE( ( b != i ) );
    REQUIRE( ( i != b ) );
    boolean b2 = true;
    integer i2 = 1;
    REQUIRE( ( b2 != i2 ) );
    REQUIRE( ( i2 != b2 ) );
  }

  // This section has some extra parens around the comparisons to
  // prevent Catch's expression templates from messing up our
  // comparison overload selection.
  SECTION( "boolean with floating" ) {
    boolean  b = true;
    floating f = 1.;
    REQUIRE( ( b != f ) );
    REQUIRE( ( f != b ) );
  }

  SECTION( "lightuserdata with self" ) {
    lightuserdata lud1 = C.newuserdata( 10 );
    lightuserdata lud2 = C.newuserdata( 10 );
    void*         p1   = lud1.get();
    void*         p2   = lud2.get();
    REQUIRE( lud1 == lud1 );
    REQUIRE( lud2 == lud2 );
    REQUIRE( lud1 != lud2 );
    REQUIRE( lud2 != lud1 );
    REQUIRE( lud1 == p1 );
    REQUIRE( lud1 != p2 );
    REQUIRE( p1 == lud1 );
    REQUIRE( p2 != lud1 );
    int x;
    REQUIRE( lud1 != &x );
    string s;
    REQUIRE( lud1 != &s );
    C.pop( 2 );
  }

  SECTION( "lightuserdata with integer" ) {
    lightuserdata lud = C.newuserdata( 10 );
    integer       i   = 1;
    REQUIRE( lud != i );
    REQUIRE( i != lud );
    REQUIRE( lud != 1 );
    REQUIRE( 1 != lud );
  }

  SECTION( "lightuserdata with floating" ) {
    lightuserdata lud = C.newuserdata( 10 );
    floating      f   = 1.;
    REQUIRE( lud != f );
    REQUIRE( f != lud );
    REQUIRE( lud != 1. );
    REQUIRE( 1. != lud );
  }

  SECTION( "integer with integer" ) {
    integer i1 = 1;
    integer i2 = -4;
    REQUIRE( i1 == i1 );
    REQUIRE( i2 == i2 );
    REQUIRE( i1 != i2 );
    REQUIRE( i2 != i1 );
    REQUIRE( i1 == 1 );
    REQUIRE( i1 != 0 );
    REQUIRE( 1 == i1 );
    REQUIRE( 0 != i1 );
  }

  SECTION( "integer with floating" ) {
    integer  i1 = 1;
    integer  i2 = -4;
    floating f1 = 1.0;
    floating f2 = -4.3;
    REQUIRE( i1 == f1 );
    REQUIRE( f1 == i1 );
    REQUIRE( i2 != f2 );
    REQUIRE( f2 != i2 );
    integer  iz = 0;
    floating fz = 0.;
    REQUIRE( iz == fz );
    REQUIRE( fz == iz );
  }

  SECTION( "floating with floating" ) {
    floating f1 = 1.3;
    floating f2 = -4.5;
    REQUIRE( f1 == f1 );
    REQUIRE( f2 == f2 );
    REQUIRE( f1 != f2 );
    REQUIRE( f2 != f1 );
    REQUIRE( f1 == 1.3 );
    REQUIRE( f1 != 0.0 );
    REQUIRE( 1.3 == f1 );
    REQUIRE( 0.0 != f1 );
  }
}

} // namespace
} // namespace lua
