/****************************************************************
**any.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-24.
*
* Description: Unit tests for the src/luapp/any.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/any.hpp"

// Testing
#include "test/luapp/common.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

LUA_TEST_CASE( "[any] any equality" ) {
  boolean       b   = true;
  integer       i   = 1;
  floating      f   = 2.3;
  lightuserdata lud = C.newuserdata( 10 );
  C.pop();

  C.newtable();
  any o1( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  C.newtable();
  any o2( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  any o3 = o1;

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

LUA_TEST_CASE( "[any] can push moved-from ref as nil" ) {
  C.newtable();
  any r( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  any r2 = std::move( r );
  push( L, r );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::nil );
  C.pop();
}

LUA_TEST_CASE( "[any] any create/push/gc" ) {
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
    C.newtable();
    int ref = C.ref_registry();
    REQUIRE( C.stack_size() == 0 );
    any o( C.this_cthread(), ref );
    push( C.this_cthread(), o );
    REQUIRE( C.type_of( -1 ) == type::table );
    REQUIRE( C.stack_size() == 1 );
    create_metatable();
    verify_collect( false );
  }
  // *** This is the test!
  verify_collect( true );
}

LUA_TEST_CASE( "[any] any copy --> no collect" ) {
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
    C.newtable();
    any t0( C.this_cthread(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );
    {
      C.newtable();
      int ref = C.ref_registry();
      REQUIRE( C.stack_size() == 0 );
      any o( C.this_cthread(), ref );
      push( C.this_cthread(), o );
      REQUIRE( C.type_of( -1 ) == type::table );
      REQUIRE( C.stack_size() == 1 );
      create_metatable();
      verify_collect( false );
      // This copy should prevent it from being garbage col-
      // lected, since it should take out a new reference on the
      // same object.
      t0 = o;
    }
    // *** This is the test!
    verify_collect( false );
  }

  // *** This is the test! The copy has now gone out of scope,
  // so we should collect the original object.
  verify_collect( true );
}

LUA_TEST_CASE( "[any] any indexing" ) {
  st["x"]      = st.table.create();
  st["x"]["y"] = 5;

  any a = st["x"];
  REQUIRE( a["y"] == 5 );
}

LUA_TEST_CASE( "[any] as" ) {
  st["x"]      = st.table.create();
  st["x"]["y"] = 5;

  any a = st["x"];
  any i = a.as<table>()["y"];
  REQUIRE( i.as<int>() == 5 );
}

} // namespace
} // namespace lua
