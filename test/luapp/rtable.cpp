/****************************************************************
**rtable.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: Unit tests for the src/luapp/table.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/rtable.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/as.hpp"
#include "src/luapp/func-push.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace lua {
namespace {

using namespace std;

using ::base::valid;

/****************************************************************
** table objects
*****************************************************************/
LUA_TEST_CASE( "[table] table equality" ) {
  boolean       b   = true;
  integer       i   = 1;
  floating      f   = 2.3;
  lightuserdata lud = C.newuserdata( 10 );
  C.pop();

  C.newtable();
  table o1( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  C.newtable();
  table o2( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  table o3 = o1;

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

LUA_TEST_CASE( "[table] table create/push/gc" ) {
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
    table o( C.this_cthread(), ref );
    push( C.this_cthread(), o );
    REQUIRE( C.type_of( -1 ) == type::table );
    REQUIRE( C.stack_size() == 1 );
    create_metatable();
    verify_collect( false );
  }
  // *** This is the test!
  verify_collect( true );
}

LUA_TEST_CASE( "[table] table copy --> no collect" ) {
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
    table t0( C.this_cthread(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );
    {
      C.newtable();
      int ref = C.ref_registry();
      REQUIRE( C.stack_size() == 0 );
      table o( C.this_cthread(), ref );
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

LUA_TEST_CASE( "[rtable] table index" ) {
  cthread L = C.this_cthread();

  auto new_table = [&] {
    C.newtable();
    table t( L, C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );
    return t;
  };

  table G = new_table();
  push( L, G );
  C.setglobal( "G" );
  REQUIRE( C.stack_size() == 0 );

  G[7.7] = "target";

  G["hello"]    = new_table();
  G["hello"][5] = new_table();
  // Create circular reference.
  G["hello"][5]["foo"] = G;

  REQUIRE( C.dostring( R"(
    return G.hello[5].foo.hello[5]['foo'][7.7]
  )" ) == valid );

  REQUIRE( C.get<string>( -1 ) == "target" );
  C.pop();
}

LUA_TEST_CASE( "[table] cpp from cpp via lua" ) {
  C.openlibs();

  st["go"] = st.table.create();
  push( L, st["go"] );
  REQUIRE( C.stack_size() == 1 );
  C.newtable();
  REQUIRE( C.stack_size() == 2 );
  C.setmetatable( -2 );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.getmetatable( -1 ) );
  REQUIRE( C.stack_size() == 2 );
  table metatable = table( L, C.ref_registry() );
  REQUIRE( C.stack_size() == 1 );
  C.pop();
  REQUIRE( C.stack_size() == 0 );

  metatable["__call"] = [&]( any a, int n, string const& s,
                             double d ) {
    CHECK( a == st["go"] );
    return fmt::format( "args: n={}, s='{}', d={}", n, s, d );
  };

  table t = as<table>( st["go"] );

  any a = t( 3, "hello", 3.6 );
  REQUIRE( a == "args: n=3, s='hello', d=3.6" );

  string s = t.call<string>( 3, "hello", 3.6 );
  REQUIRE( s == "args: n=3, s='hello', d=3.6" );

  REQUIRE( t( 4, "hello", 3.6 ) ==
           "args: n=4, s='hello', d=3.6" );

  REQUIRE( t( 3, "hello", 3.7 ) ==
           "args: n=3, s='hello', d=3.7" );

  REQUIRE( t.pcall<rstring>( 3, "hello", 3.7 ) ==
           "args: n=3, s='hello', d=3.7" );
}

LUA_TEST_CASE( "[table] table create_or_get" ) {
  table t1 = table::create_or_get( st["t1"] );
  table t2 = table::create_or_get( st["t1"] );
  REQUIRE( t1 == t2 );
  table t3 = table::create_or_get( st["t1"]["t3"] );
  REQUIRE( t3 != t1 );
  table t4 = table::create_or_get( st["t1"]["t3"] );
  REQUIRE( t3 == t4 );

  st["foo"] = [&] {
    st["number"] = 5.5;
    return table::create_or_get( st["number"] );
  };

  char const* err =
      "expected either a table or nil but found an object of "
      "type number.\n"
      "stack traceback:\n"
      "\t[C]: in ?";

  REQUIRE( st["foo"].pcall<table>() ==
           lua_unexpected<table>( err ) );
}

} // namespace
} // namespace lua
