/****************************************************************
**thing.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-09.
*
* Description: Unit tests for the src/luapp/thing.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/thing.hpp"

// luapp
#include "src/luapp/c-api.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::e_lua_type );
FMT_TO_CATCH( ::lua::thing );

namespace lua {
namespace {

using namespace std;

using ::base::valid;
using ::Catch::Matches;

/****************************************************************
** value equality
*****************************************************************/
TEST_CASE( "[value types] equality" ) {
  c_api C;

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

/****************************************************************
** reference objects
*****************************************************************/
TEST_CASE( "[reference] reference equality" ) {
  c_api C;

  boolean       b   = true;
  integer       i   = 1;
  floating      f   = 2.3;
  lightuserdata lud = C.newuserdata( 10 );
  C.pop();

  SECTION( "table" ) {
    C.newtable();
    table o1( C.state(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );

    C.newtable();
    table o2( C.state(), C.ref_registry() );
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

  SECTION( "userdata" ) {
    C.newuserdata( 1024 );
    userdata o1( C.state(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );

    C.newuserdata( 1024 );
    userdata o2( C.state(), C.ref_registry() );
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

  SECTION( "lfunction" ) {
    C.push( []( lua_State* ) -> int { return 0; } );
    lfunction o1( C.state(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );

    C.push( []( lua_State* ) -> int { return 0; } );
    lfunction o2( C.state(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );

    lfunction o3 = o1;

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

  SECTION( "lthread" ) {
    (void)C.newthread();
    lthread o1( C.state(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );

    (void)C.newthread();
    lthread o2( C.state(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );

    lthread o3 = o1;

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

  SECTION( "lstring" ) {
    C.push( "hello" );
    lstring o1( C.state(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );

    C.push( "hello" );
    lstring o2( C.state(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );

    C.push( "world" );
    lstring o3( C.state(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );

    lstring o4 = o1;

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
}

TEST_CASE( "[reference] reference create/push/gc" ) {
  c_api C;

  auto create_metatable = [&] {
    C.newtable();
    C.push( []( lua_State* L ) -> int {
      c_api C = c_api::view( L );
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

  SECTION( "table" ) {
    {
      C.newtable();
      int ref = C.ref_registry();
      REQUIRE( C.stack_size() == 0 );
      table o( C.state(), ref );
      o.push();
      REQUIRE( C.type_of( -1 ) == e_lua_type::table );
      REQUIRE( C.stack_size() == 1 );
      create_metatable();
      verify_collect( false );
    }
    // *** This is the test!
    verify_collect( true );
  }

  SECTION( "lfunction" ) {
    {
      // Need to track gc in the upvalue for a function, since a
      // function can't have a metatable directly.
      C.newtable();
      C.pushvalue( -1 );
      REQUIRE( C.stack_size() == 2 );
      create_metatable();
      REQUIRE( C.stack_size() == 1 );
      C.push( []( lua_State* ) -> int { return 0; },
              /*upvalues=*/1 );
      int ref = C.ref_registry();
      REQUIRE( C.stack_size() == 0 );
      lfunction o( C.state(), ref );
      o.push();
      REQUIRE( C.type_of( -1 ) == e_lua_type::function );
      REQUIRE( C.stack_size() == 1 );
      create_metatable();
      verify_collect( false );
    }
    // *** This is the test!
    verify_collect( true );
  }

  SECTION( "userdata" ) {
    {
      C.newuserdata( 1024 );
      int ref = C.ref_registry();
      REQUIRE( C.stack_size() == 0 );
      userdata o( C.state(), ref );
      o.push();
      REQUIRE( C.type_of( -1 ) == e_lua_type::userdata );
      REQUIRE( C.stack_size() == 1 );
      create_metatable();
      verify_collect( false );
    }
    // *** This is the test!
    verify_collect( true );
  }
}

TEST_CASE( "[reference] reference copy --> no collect" ) {
  c_api C;

  auto create_metatable = [&] {
    C.newtable();
    C.push( []( lua_State* L ) -> int {
      c_api C = c_api::view( L );
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

  SECTION( "table, copy+no collect" ) {
    C.newtable();
    table t0( C.state(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );
    {
      C.newtable();
      int ref = C.ref_registry();
      REQUIRE( C.stack_size() == 0 );
      table o( C.state(), ref );
      o.push();
      REQUIRE( C.type_of( -1 ) == e_lua_type::table );
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

  SECTION( "lfunction" ) {
    C.push( []( lua_State* ) -> int { return 0; } );
    lfunction f0( C.state(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );
    {
      // Need to track gc in the upvalue for a function, since a
      // function can't have a metatable directly.
      C.newtable();
      C.pushvalue( -1 );
      REQUIRE( C.stack_size() == 2 );
      create_metatable();
      REQUIRE( C.stack_size() == 1 );
      C.push( []( lua_State* ) -> int { return 0; },
              /*upvalues=*/1 );
      int ref = C.ref_registry();
      REQUIRE( C.stack_size() == 0 );
      lfunction o( C.state(), ref );
      o.push();
      REQUIRE( C.type_of( -1 ) == e_lua_type::function );
      REQUIRE( C.stack_size() == 1 );
      create_metatable();
      verify_collect( false );
      // This copy should prevent it from being garbage col-
      // lected, since it should take out a new reference on the
      // same object.
      f0 = o;
    }
    // *** This is the test!
    verify_collect( false );
  }

  SECTION( "userdata" ) {
    C.newuserdata( 10 );
    userdata u0( C.state(), C.ref_registry() );
    REQUIRE( C.stack_size() == 0 );
    {
      C.newuserdata( 1024 );
      int ref = C.ref_registry();
      REQUIRE( C.stack_size() == 0 );
      userdata o( C.state(), ref );
      o.push();
      REQUIRE( C.type_of( -1 ) == e_lua_type::userdata );
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

TEST_CASE( "[lstring] as_cpp / string literal cmp" ) {
  c_api C;
  C.push( "hello" );
  int ref = C.ref_registry();
  REQUIRE( C.stack_size() == 0 );
  lstring s( C.state(), ref );

  REQUIRE( s == "hello" );
  REQUIRE( "hello" == s );
  REQUIRE( s != "world" );
  REQUIRE( "world" != s );

  REQUIRE( s.as_cpp() == "hello" );
}

/****************************************************************
** thing
*****************************************************************/
TEST_CASE( "[thing] static checks" ) {
  static_assert( is_default_constructible_v<thing> );

  static_assert( is_nothrow_move_constructible_v<thing> );
  static_assert( is_nothrow_move_assignable_v<thing> );
  // FIXME: figure out why these are not no-throw.
  static_assert( is_copy_constructible_v<thing> );
  static_assert( is_copy_assignable_v<thing> );

  // This is to prevent assigning strings or string literals. We
  // can't use them to create Lua strings since we may not have
  // an L, and we probably don't want to create light userdatas
  // from them, so just better to prevent it.
  static_assert( !is_assignable_v<thing, char const( & )[6]> );
  static_assert( !is_assignable_v<thing, char const( * )[6]> );
  static_assert( !is_assignable_v<thing, char const*> );
  static_assert( !is_assignable_v<thing, char( & )[6]> );
  static_assert( !is_assignable_v<thing, char( * )[6]> );
  static_assert( !is_assignable_v<thing, char*> );
  static_assert( !is_assignable_v<thing, std::string> );
  static_assert( !is_assignable_v<thing, std::string_view> );

  // This is kind of weird, so disallow it.
  static_assert( !is_assignable_v<thing, void const*> );

  // Make sure this is allowed as it is for light userdata.
  static_assert( is_assignable_v<thing, void*> );

  static_assert( is_nothrow_convertible_v<thing, bool> );
}

TEST_CASE( "[thing] defaults to nil + equality with nil" ) {
  thing th;
  REQUIRE( th == nil );
  REQUIRE( th.type() == e_lua_type::nil );
  REQUIRE( !th );
}

TEST_CASE( "[thing] nil assignment / equality" ) {
  thing th = nil;
  REQUIRE( th.type() == e_lua_type::nil );
  REQUIRE( th.index() == 0 );
  REQUIRE( th == nil );
  REQUIRE( !th );
  th = nil;
  REQUIRE( th.type() == e_lua_type::nil );
  REQUIRE( th.index() == 0 );
  REQUIRE( th == nil );
  REQUIRE( !th );
}

TEST_CASE( "[thing] bool assignment / equality" ) {
  thing th = true;
  REQUIRE( th );
  REQUIRE( th.type() == e_lua_type::boolean );
  REQUIRE( th.index() == 1 );
  REQUIRE( th == true );
  th = false;
  REQUIRE( th != true );
  REQUIRE( th == false );
  REQUIRE( th.index() == 1 );
  REQUIRE( !th );
}

TEST_CASE( "[thing] int assignment / equality" ) {
  thing th = 5;
  REQUIRE( th );
  REQUIRE( th != nil );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 3 );
  REQUIRE( th == 5 );
  th = 6;
  REQUIRE( th );
  REQUIRE( th != 5 );
  REQUIRE( th == 6 );
  th = 7L;
  REQUIRE( th == 7L );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 3 );
  th = -1;
  REQUIRE( th );
  REQUIRE( th == -1 );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 3 );
  th = 0;
  REQUIRE( th ); // Lua's rules.
  REQUIRE( th == 0 );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 3 );
}

TEST_CASE( "[thing] double assignment / equality" ) {
  thing th = 5.5;
  REQUIRE( th );
  REQUIRE( th != nil );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 4 );
  REQUIRE( th == 5.5 );
  th = 6.5;
  REQUIRE( th );
  REQUIRE( th != 5.5 );
  REQUIRE( th == 6.5 );
  th = -1.3;
  REQUIRE( th );
  REQUIRE( th == -1.3 );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 4 );
  th = 0.0;
  REQUIRE( th ); // Lua's rules.
  REQUIRE( th == 0.0 );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.index() == 4 );
}

TEST_CASE( "[thing] lightuserdata assignment / equality" ) {
  thing th = nullptr;
  REQUIRE( th ); // Lua's rules.
  REQUIRE( th == nullptr );
  REQUIRE( th != nil );
  REQUIRE( th.type() == e_lua_type::lightuserdata );
  REQUIRE( th.index() == 2 );

  int x = 3;
  th    = (void*)&x;
  REQUIRE( th );
  REQUIRE( th != nullptr );
  REQUIRE( th != nil );
  REQUIRE( th.type() == e_lua_type::lightuserdata );
  REQUIRE( th.index() == 2 );
  REQUIRE( th == (void*)&x );
}

TEST_CASE( "[thing] string assignment / equality" ) {
  c_api C;
  C.push( "hello" );
  lstring s1( C.state(), C.ref_registry() );
  thing   th1 = s1;
  C.push( "hello" );
  lstring s2( C.state(), C.ref_registry() );
  thing   th2 = s2;
  C.push( "world" );
  lstring s3( C.state(), C.ref_registry() );
  thing   th3 = s3;
  REQUIRE( th1 );
  REQUIRE( th1 == s1 );
  REQUIRE( th1 != nil );
  REQUIRE( th1.type() == e_lua_type::string );
  REQUIRE( th1.index() == 5 );

  REQUIRE( th1 == th2 );
  REQUIRE( th2 == th1 );
  REQUIRE( th1 != th3 );
  REQUIRE( th3 != th1 );
}

TEST_CASE( "[thing] table assignment / equality" ) {
  c_api C;
  C.newtable();
  table t1( C.state(), C.ref_registry() );
  thing th1 = t1;
  table t2  = t1;
  thing th2 = t2;
  C.newtable();
  table t3( C.state(), C.ref_registry() );
  thing th3 = t3;
  REQUIRE( th1 );
  REQUIRE( th1 == t1 );
  REQUIRE( th1 != nil );
  REQUIRE( th1.type() == e_lua_type::table );
  REQUIRE( th1.index() == 6 );

  REQUIRE( th1 == th2 );
  REQUIRE( th2 == th1 );
  REQUIRE( th1 != th3 );
  REQUIRE( th3 != th1 );
}

TEST_CASE( "[thing] thread assignment / equality" ) {
  c_api C;
  (void)C.newthread();
  lthread o1( C.state(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );
  thing th1 = o1;

  (void)C.newthread();
  lthread o2( C.state(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );
  thing th2 = o2;

  REQUIRE( th1 != th2 );
  thing th3 = th2;
  REQUIRE( th3 == th2 );
}

TEST_CASE( "[thing] thing inequality" ) {
  c_api C;

  vector<thing> v;
  v.push_back( 5 );
  v.push_back( 5.5 );
  v.push_back( true );
  v.push_back( (void*)"hello" );
  v.push_back( nil );

  C.push( "hello" );
  lstring s( C.state(), C.ref_registry() );
  v.push_back( s );

  C.newtable();
  table t( C.state(), C.ref_registry() );
  v.push_back( t );

  C.push( []( lua_State* ) -> int { return 0; } );
  lfunction f( C.state(), C.ref_registry() );
  v.push_back( f );

  C.newuserdata( 10 );
  userdata ud( C.state(), C.ref_registry() );
  v.push_back( ud );

  (void)C.newthread();
  lthread lthr( C.state(), C.ref_registry() );
  v.push_back( lthr );

  for( int i = 0; i < int( v.size() ); ++i ) {
    for( int j = 0; j < int( v.size() ); ++j ) {
      if( i == j ) {
        REQUIRE( v[i] == v[j] );
        REQUIRE( v[j] == v[i] );
      } else {
        REQUIRE( v[i] != v[j] );
        REQUIRE( v[j] != v[i] );
      }
    }
  }
}

TEST_CASE( "[thing] thing inequality convertiblae" ) {
  c_api C;

  thing th1 = 5;
  thing th2 = 5.0;
  thing th3 = true;

  REQUIRE( th1 == th2 );
  REQUIRE( th2 == th1 );
  REQUIRE( th3 != th1 );
  REQUIRE( th3 != th2 );
  REQUIRE( th1 != th3 );
  REQUIRE( th2 != th3 );

  C.push( "5.3" );
  lstring s1( C.state(), C.ref_registry() );
  C.push( "5.0" );
  lstring s2( C.state(), C.ref_registry() );

  // No auto conversions on comparison apparently (verified in
  // Lua repl).
  REQUIRE( thing( s1 ) != 5.3 );
  REQUIRE( thing( s1 ) != 5 );
  REQUIRE( thing( s2 ) != 5 );
  REQUIRE( thing( s2 ) != 5.0 );

  REQUIRE( thing( 5.3 ) != thing( s1 ) );
  REQUIRE( thing( 5 ) != thing( s1 ) );
  REQUIRE( thing( 5.3 ) != thing( s2 ) );
  REQUIRE( thing( 5 ) != thing( s2 ) );
}

TEST_CASE( "[thing] fmt/to_str" ) {
  c_api C;

  thing th;
  REQUIRE( th.type() == e_lua_type::nil );
  REQUIRE( fmt::format( "{}", th ) == "nil" );

  th = true;
  REQUIRE( th.type() == e_lua_type::boolean );
  REQUIRE( th == true );
  REQUIRE( fmt::format( "{}", th ) == "true" );

  th = false;
  REQUIRE( th.type() == e_lua_type::boolean );
  REQUIRE( th == false );
  REQUIRE( fmt::format( "{}", th ) == "false" );

  th = 42;
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th == 42 );
  REQUIRE( fmt::format( "{}", th ) == "42" );

  th = 3.5;
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th == 3.5 );
  REQUIRE( fmt::format( "{}", th ) == "3.5" );

  void* p = nullptr;
  th      = p;
  REQUIRE( th.type() == e_lua_type::lightuserdata );
  REQUIRE( th == nullptr );
  REQUIRE( fmt::format( "{}", th ) == "userdata: (nil)" );
  int x = 3;
  p     = &x;
  th    = p;
  REQUIRE( th.type() == e_lua_type::lightuserdata );
  REQUIRE( th == (void*)&x );
  REQUIRE_THAT( fmt::format( "{}", th ),
                Matches( "userdata: 0x[0-9a-z]+" ) );

  // strings.
  C.push( "hello" );
  th = lstring( C.state(), C.ref_registry() );
  REQUIRE( fmt::format( "{}", th ) == "hello" );

  // tables.
  C.newtable();
  th = table( C.state(), C.ref_registry() );
  REQUIRE_THAT( fmt::format( "{}", th ),
                Matches( "table: 0x[0-9a-z]+" ) );

  // function.
  C.push( []( lua_State* ) -> int { return 0; } );
  th = lfunction( C.state(), C.ref_registry() );
  REQUIRE_THAT( fmt::format( "{}", th ),
                Matches( "function: 0x[0-9a-z]+" ) );

  // userdata.
  C.newuserdata( 10 );
  th = userdata( C.state(), C.ref_registry() );
  REQUIRE_THAT( fmt::format( "{}", th ),
                Matches( "userdata: 0x[0-9a-z]+" ) );

  // thread.
  (void)C.newthread();
  th = lthread( C.state(), C.ref_registry() );
  REQUIRE_THAT( fmt::format( "{}", th ),
                Matches( "thread: 0x[0-9a-z]+" ) );
}

TEST_CASE( "[thing] thing::push" ) {
  c_api C;

  SECTION( "nil" ) {
    thing th = nil;
    th.push( C.state() );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::nil );
    C.pop();
  }
  SECTION( "boolean" ) {
    thing th = true;
    th.push( C.state() );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::boolean );
    REQUIRE( C.get<bool>( -1 ) == true );
    C.pop();
  }
  SECTION( "lightuserdata" ) {
    int   x  = 0;
    thing th = (void*)&x;
    th.push( C.state() );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::lightuserdata );
    REQUIRE( C.get<void*>( -1 ) == &x );
    C.pop();
  }
  SECTION( "integer" ) {
    thing th = 5;
    th.push( C.state() );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::number );
    REQUIRE( C.get<int>( -1 ) == 5 );
    C.pop();
  }
  SECTION( "floating" ) {
    thing th = 5.5;
    th.push( C.state() );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::number );
    REQUIRE( C.get<double>( -1 ) == 5.5 );
    C.pop();
  }
  SECTION( "lstring" ) {
    C.push( "hello" );
    thing th = lstring( C.state(), C.ref_registry() );
    th.push( C.state() );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::string );
    REQUIRE( C.get<string>( -1 ) == "hello" );
    C.pop();
  }
  SECTION( "table" ) {
    C.newtable();
    thing th = table( C.state(), C.ref_registry() );
    th.push( C.state() );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::table );
    REQUIRE( th == table( C.state(), C.ref_registry() ) );
  }
  SECTION( "lfunction" ) {
    C.push( []( lua_State* ) -> int { return 0; } );
    thing th = lfunction( C.state(), C.ref_registry() );
    th.push( C.state() );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::function );
    REQUIRE( th == lfunction( C.state(), C.ref_registry() ) );
  }
  SECTION( "userdata" ) {
    C.newuserdata( 10 );
    thing th = userdata( C.state(), C.ref_registry() );
    th.push( C.state() );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::userdata );
    REQUIRE( th == userdata( C.state(), C.ref_registry() ) );
  }
  SECTION( "lthread" ) {
    (void)C.newthread();
    thing th = lthread( C.state(), C.ref_registry() );
    th.push( C.state() );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::thread );
    REQUIRE( th == lthread( C.state(), C.ref_registry() ) );
  }

  REQUIRE( C.stack_size() == 0 );
}

TEST_CASE( "[thing] thing::pop" ) {
  c_api C;

  thing th;

  // nil
  C.push( nil );
  th = thing::pop( C.state() );
  REQUIRE( th.type() == e_lua_type::nil );
  REQUIRE( th.holds<nil_t>() );
  REQUIRE( th == nil );

  // bool
  C.push( true );
  th = thing::pop( C.state() );
  REQUIRE( th.type() == e_lua_type::boolean );
  REQUIRE( th.holds<boolean>() );
  REQUIRE( th == true );

  // lightuserdata
  C.push( (void*)&C );
  th = thing::pop( C.state() );
  REQUIRE( th.type() == e_lua_type::lightuserdata );
  REQUIRE( th.holds<lightuserdata>() );
  REQUIRE( th == (void*)&C );

  // int
  C.push( 5 );
  th = thing::pop( C.state() );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.holds<integer>() );
  REQUIRE( th == 5 );

  // double
  C.push( 5.0 );
  th = thing::pop( C.state() );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.holds<floating>() );
  REQUIRE( th == 5.0 );
  C.push( 5.5 );
  th = thing::pop( C.state() );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.holds<floating>() );
  REQUIRE( th == 5.5 );

  // string
  C.push( "hello" );
  th = thing::pop( C.state() );
  REQUIRE( th.type() == e_lua_type::string );
  REQUIRE( th.holds<lstring>() );
  REQUIRE( th == "hello" );

  // table
  C.newtable();
  th = thing::pop( C.state() );
  REQUIRE( th.type() == e_lua_type::table );
  REQUIRE( th.holds<table>() );

  // function
  C.push( []( lua_State* ) -> int { return 0; } );
  th = thing::pop( C.state() );
  REQUIRE( th.type() == e_lua_type::function );
  REQUIRE( th.holds<lfunction>() );

  // userdata
  C.newuserdata( 10 );
  th = thing::pop( C.state() );
  REQUIRE( th.type() == e_lua_type::userdata );
  REQUIRE( th.holds<userdata>() );

  // thread
  (void)C.newthread();
  th = thing::pop( C.state() );
  REQUIRE( th.type() == e_lua_type::thread );
  REQUIRE( th.holds<lthread>() );
}

} // namespace
} // namespace lua
