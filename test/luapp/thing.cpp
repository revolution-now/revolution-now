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

// Testing
#include "test/luapp/common.hpp"

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
** thing
*****************************************************************/
LUA_TEST_CASE( "[thing] static checks" ) {
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

LUA_TEST_CASE( "[thing] defaults to nil + equality with nil" ) {
  thing th;
  REQUIRE( th == nil );
  REQUIRE( th.type() == e_lua_type::nil );
  REQUIRE( !th );
}

LUA_TEST_CASE( "[thing] nil assignment / equality" ) {
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

LUA_TEST_CASE( "[thing] bool assignment / equality" ) {
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

LUA_TEST_CASE( "[thing] int assignment / equality" ) {
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

LUA_TEST_CASE( "[thing] double assignment / equality" ) {
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

LUA_TEST_CASE( "[thing] lightuserdata assignment / equality" ) {
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

LUA_TEST_CASE( "[thing] string assignment / equality" ) {
  C.push( "hello" );
  rstring s1( C.this_cthread(), C.ref_registry() );
  thing   th1 = s1;
  C.push( "hello" );
  rstring s2( C.this_cthread(), C.ref_registry() );
  thing   th2 = s2;
  C.push( "world" );
  rstring s3( C.this_cthread(), C.ref_registry() );
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

LUA_TEST_CASE( "[thing] table assignment / equality" ) {
  C.newtable();
  table t1( C.this_cthread(), C.ref_registry() );
  thing th1 = t1;
  table t2  = t1;
  thing th2 = t2;
  C.newtable();
  table t3( C.this_cthread(), C.ref_registry() );
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

LUA_TEST_CASE( "[thing] thread assignment / equality" ) {
  (void)C.newthread();
  rthread o1( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );
  thing th1 = o1;

  (void)C.newthread();
  rthread o2( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );
  thing th2 = o2;

  REQUIRE( th1 != th2 );
  thing th3 = th2;
  REQUIRE( th3 == th2 );
}

LUA_TEST_CASE( "[thing] thing inequality" ) {
  vector<thing> v;
  v.push_back( 5 );
  v.push_back( 5.5 );
  v.push_back( true );
  v.push_back( (void*)"hello" );
  v.push_back( nil );

  C.push( "hello" );
  rstring s( C.this_cthread(), C.ref_registry() );
  v.push_back( s );

  C.newtable();
  table t( C.this_cthread(), C.ref_registry() );
  v.push_back( t );

  C.push( []( lua_State* ) -> int { return 0; } );
  rfunction f( C.this_cthread(), C.ref_registry() );
  v.push_back( f );

  C.newuserdata( 10 );
  userdata ud( C.this_cthread(), C.ref_registry() );
  v.push_back( ud );

  (void)C.newthread();
  rthread lthr( C.this_cthread(), C.ref_registry() );
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

LUA_TEST_CASE( "[thing] thing inequality convertiblae" ) {
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
  rstring s1( C.this_cthread(), C.ref_registry() );
  C.push( "5.0" );
  rstring s2( C.this_cthread(), C.ref_registry() );

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

LUA_TEST_CASE( "[thing] fmt/to_str" ) {
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
  th = rstring( C.this_cthread(), C.ref_registry() );
  REQUIRE( fmt::format( "{}", th ) == "hello" );

  // tables.
  C.newtable();
  th = table( C.this_cthread(), C.ref_registry() );
  REQUIRE_THAT( fmt::format( "{}", th ),
                Matches( "table: 0x[0-9a-z]+" ) );

  // function.
  C.push( []( lua_State* ) -> int { return 0; } );
  th = rfunction( C.this_cthread(), C.ref_registry() );
  REQUIRE_THAT( fmt::format( "{}", th ),
                Matches( "function: 0x[0-9a-z]+" ) );

  // userdata.
  C.newuserdata( 10 );
  th = userdata( C.this_cthread(), C.ref_registry() );
  REQUIRE_THAT( fmt::format( "{}", th ),
                Matches( "userdata: 0x[0-9a-z]+" ) );

  // thread.
  (void)C.newthread();
  th = rthread( C.this_cthread(), C.ref_registry() );
  REQUIRE_THAT( fmt::format( "{}", th ),
                Matches( "thread: 0x[0-9a-z]+" ) );
}

LUA_TEST_CASE( "[thing] thing::push" ) {
  SECTION( "nil" ) {
    thing th = nil;
    push( C.this_cthread(), th );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::nil );
    C.pop();
  }
  SECTION( "boolean" ) {
    thing th = true;
    push( C.this_cthread(), th );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::boolean );
    REQUIRE( C.get<bool>( -1 ) == true );
    C.pop();
  }
  SECTION( "lightuserdata" ) {
    int   x  = 0;
    thing th = (void*)&x;
    push( C.this_cthread(), th );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::lightuserdata );
    REQUIRE( C.get<void*>( -1 ) == &x );
    C.pop();
  }
  SECTION( "integer" ) {
    thing th = 5;
    push( C.this_cthread(), th );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::number );
    REQUIRE( C.get<int>( -1 ) == 5 );
    C.pop();
  }
  SECTION( "floating" ) {
    thing th = 5.5;
    push( C.this_cthread(), th );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::number );
    REQUIRE( C.get<double>( -1 ) == 5.5 );
    C.pop();
  }
  SECTION( "rstring" ) {
    C.push( "hello" );
    thing th = rstring( C.this_cthread(), C.ref_registry() );
    push( C.this_cthread(), th );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::string );
    REQUIRE( C.get<string>( -1 ) == "hello" );
    C.pop();
  }
  SECTION( "table" ) {
    C.newtable();
    thing th = table( C.this_cthread(), C.ref_registry() );
    push( C.this_cthread(), th );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::table );
    REQUIRE( th == table( C.this_cthread(), C.ref_registry() ) );
  }
  SECTION( "rfunction" ) {
    C.push( []( lua_State* ) -> int { return 0; } );
    thing th = rfunction( C.this_cthread(), C.ref_registry() );
    push( C.this_cthread(), th );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::function );
    REQUIRE( th ==
             rfunction( C.this_cthread(), C.ref_registry() ) );
  }
  SECTION( "userdata" ) {
    C.newuserdata( 10 );
    thing th = userdata( C.this_cthread(), C.ref_registry() );
    push( C.this_cthread(), th );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::userdata );
    REQUIRE( th ==
             userdata( C.this_cthread(), C.ref_registry() ) );
  }
  SECTION( "rthread" ) {
    (void)C.newthread();
    thing th = rthread( C.this_cthread(), C.ref_registry() );
    push( C.this_cthread(), th );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == e_lua_type::thread );
    REQUIRE( th ==
             rthread( C.this_cthread(), C.ref_registry() ) );
  }

  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[thing] thing::pop" ) {
  thing th;

  // nil
  C.push( nil );
  th = thing::pop( C.this_cthread() );
  REQUIRE( th.type() == e_lua_type::nil );
  REQUIRE( th.holds<nil_t>() );
  REQUIRE( th == nil );

  // bool
  C.push( true );
  th = thing::pop( C.this_cthread() );
  REQUIRE( th.type() == e_lua_type::boolean );
  REQUIRE( th.holds<boolean>() );
  REQUIRE( th == true );

  // lightuserdata
  C.push( (void*)&C );
  th = thing::pop( C.this_cthread() );
  REQUIRE( th.type() == e_lua_type::lightuserdata );
  REQUIRE( th.holds<lightuserdata>() );
  REQUIRE( th == (void*)&C );

  // int
  C.push( 5 );
  th = thing::pop( C.this_cthread() );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.holds<integer>() );
  REQUIRE( th == 5 );

  // double
  C.push( 5.0 );
  th = thing::pop( C.this_cthread() );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.holds<floating>() );
  REQUIRE( th == 5.0 );
  C.push( 5.5 );
  th = thing::pop( C.this_cthread() );
  REQUIRE( th.type() == e_lua_type::number );
  REQUIRE( th.holds<floating>() );
  REQUIRE( th == 5.5 );

  // string
  C.push( "hello" );
  th = thing::pop( C.this_cthread() );
  REQUIRE( th.type() == e_lua_type::string );
  REQUIRE( th.holds<rstring>() );
  REQUIRE( th == "hello" );

  // table
  C.newtable();
  th = thing::pop( C.this_cthread() );
  REQUIRE( th.type() == e_lua_type::table );
  REQUIRE( th.holds<table>() );

  // function
  C.push( []( lua_State* ) -> int { return 0; } );
  th = thing::pop( C.this_cthread() );
  REQUIRE( th.type() == e_lua_type::function );
  REQUIRE( th.holds<rfunction>() );

  // userdata
  C.newuserdata( 10 );
  th = thing::pop( C.this_cthread() );
  REQUIRE( th.type() == e_lua_type::userdata );
  REQUIRE( th.holds<userdata>() );

  // thread
  (void)C.newthread();
  th = thing::pop( C.this_cthread() );
  REQUIRE( th.type() == e_lua_type::thread );
  REQUIRE( th.holds<rthread>() );
}

LUA_TEST_CASE( "[thing] index with thing" ) {
  thing G = st.global_table();

  REQUIRE( G.is<table>() );
  G.as<table>()[7.7] = "target";

  thing s = st.str( "hello" );

  G.as<table>()[s]    = st.new_table();
  G.as<table>()[s][5] = st.new_table();
  // Create circular reference.
  G.as<table>()["hello"][5]["foo"] = G;

  REQUIRE( C.dostring( R"(
    return hello[5].foo.hello[5]['foo'][7.7]
  )" ) == valid );

  REQUIRE( C.get<string>( -1 ) == "target" );
  C.pop();

  REQUIRE( C.stack_size() == 0 );

  table t  = G.as<table>();
  thing th = t[7.7];
  REQUIRE( th == "target" );
}

} // namespace
} // namespace lua
