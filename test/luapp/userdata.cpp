/****************************************************************
**userdata.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: Unit tests for the src/luapp/userdata.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/userdata.hpp"

// Testing
#include "test/luapp/common.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

/****************************************************************
** userdata objects
*****************************************************************/
LUA_TEST_CASE( "[userdata] userdata equality" ) {
  boolean       b   = true;
  integer       i   = 1;
  floating      f   = 2.3;
  lightuserdata lud = C.newuserdata( 10 );
  C.pop();

  C.newuserdata( 1024 );
  userdata o1( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  C.newuserdata( 1024 );
  userdata o2( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  userdata o3 = o1;

  REQUIRE( o1 != o2 );
  REQUIRE( o1 == o3 );

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

LUA_TEST_CASE( "[userdata] userdata create/push/gc" ) {
  auto create_metatable = [&] {
    C.newtable();
    C.push( []( lua_State* L ) -> int {
      c_api C( L );
      C.push( true );
      C.setglobal( "was_collected" );
      return 0;
    } );
    C.setfield( -2, "__gc" );
    C.setmetatable( -2 );
    REQUIRE( C.stack_size() >= 1 );
    C.pop();
    REQUIRE( C.stack_size() >= 0 );
  };

  auto verify_collect = [&]( bool was_collected ) {
    C.gc_collect();
    C.getglobal( "was_collected" );
    if( C.get<bool>( -1 ) != was_collected )
      base::abort_with_msg( "here" );
    C.pop();
    REQUIRE( C.stack_size() == 0 );
  };

  {
    C.newuserdata( 1024 );
    int ref = C.ref_registry();
    REQUIRE( C.stack_size() == 0 );
    userdata o( C.this_cthread(), ref );
    push( C.this_cthread(), o );
    REQUIRE( C.type_of( -1 ) == type::userdata );
    REQUIRE( C.stack_size() == 1 );
    create_metatable();
    verify_collect( false );
  }
  // *** This is the test!
  verify_collect( true );
}

LUA_TEST_CASE( "[userdata] userdata copy --> no collect" ) {
  auto create_metatable = [&] {
    C.newtable();
    C.push( []( lua_State* L ) -> int {
      c_api C( L );
      C.push( true );
      C.setglobal( "was_collected" );
      return 0;
    } );
    C.setfield( -2, "__gc" );
    C.setmetatable( -2 );
    REQUIRE( C.stack_size() >= 1 );
    C.pop();
    REQUIRE( C.stack_size() >= 0 );
  };

  auto verify_collect = [&]( bool was_collected ) {
    C.gc_collect();
    C.getglobal( "was_collected" );
    if( C.get<bool>( -1 ) != was_collected )
      base::abort_with_msg( "here" );
    C.pop();
    REQUIRE( C.stack_size() == 0 );
  };

  {
    C.newuserdata( 10 );
    userdata u0( C.this_cthread(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );
    {
      C.newuserdata( 1024 );
      int ref = C.ref_registry();
      REQUIRE( C.stack_size() == 0 );
      userdata o( C.this_cthread(), ref );
      push( C.this_cthread(), o );
      REQUIRE( C.type_of( -1 ) == type::userdata );
      REQUIRE( C.stack_size() == 1 );
      create_metatable();
      verify_collect( false );
      // This copy should prevent it from being garbage col-
      // lected, since it should take out a new reference on the
      // same object.
      u0 = o;
    }
    // *** This is the test!
    verify_collect( false );
  }

  // *** This is the test! The copy has now gone out of scope,
  // so we should collect the original object.
  verify_collect( true );
}

} // namespace
} // namespace lua
