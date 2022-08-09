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

namespace lua {

struct MyLuaOwned {};
static void to_str( MyLuaOwned const&, std::string&,
                    base::ADL_t ) {}
LUA_USERDATA_TRAITS( MyLuaOwned, owned_by_lua ){};

namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;

// This is not so much to test push/get on references, it is to
// test that maybe<T> can be pushed/popped when T is a reference.
struct Reffable {
  int x = 9;

  friend void lua_push( lua::cthread L, Reffable const& r ) {
    lua::c_api C( L );
    push( L, (void*)&r );
  }

  friend base::maybe<Reffable&> lua_get( lua::cthread L, int idx,
                                         lua::tag<Reffable&> ) {
    lua::c_api C( L );
    CHECK( C.type_of( idx ) == type::lightuserdata );
    UNWRAP_RETURN( m, C.get<void*>( idx ) );
    Reffable* p_r = static_cast<Reffable*>( m );
    return *p_r;
  }
};

LUA_TEST_CASE( "[ext-base] lua-owned" ) {
  maybe<MyLuaOwned> m;

  SECTION( "nothing" ) {
    m.reset();
    push( L, std::move( m ) );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::nil );
    C.pop();
  }

  SECTION( "something" ) {
    m.reset();
    m.emplace();
    push( L, std::move( m ) );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::userdata );
    C.pop();
  }
}

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

LUA_TEST_CASE( "[ext-base] maybe ref" ) {
  Reffable         r;
  maybe<Reffable&> m = r;

  push( L, m );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::lightuserdata );
  maybe<Reffable&> m2 = get<Reffable&>( L, -1 );
  REQUIRE( m2.has_value() );
  REQUIRE( m2->x == 9 );

  C.pop();
}

} // namespace
} // namespace lua
