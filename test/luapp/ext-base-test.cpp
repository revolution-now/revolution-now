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

struct MyLuaOwned {
  int n = 0;
};
[[maybe_unused]] static void to_str( MyLuaOwned const&,
                                     std::string&,
                                     base::tag<MyLuaOwned> ) {}
LUA_USERDATA_TRAITS( MyLuaOwned, owned_by_lua ){};

struct MyCppOwned {
  int n = 0;
};
[[maybe_unused]] static void to_str( MyCppOwned const&,
                                     std::string&,
                                     base::tag<MyCppOwned> ) {}
LUA_USERDATA_TRAITS( MyCppOwned, owned_by_lua ){};

namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;

// This is not so much to test push/get on references, it is to
// test that maybe<T> can be pushed/popped when T is a reference.
struct Reffable {
  int x = 9;

  [[maybe_unused]] friend void lua_push( lua::cthread L,
                                         Reffable const& r ) {
    lua::c_api C( L );
    push( L, (void*)&r );
  }

  friend lua_expect<Reffable&> lua_get( lua::cthread L, int idx,
                                        lua::tag<Reffable&> ) {
    lua::c_api C( L );
    CHECK( C.type_of( idx ) == type::lightuserdata );
    UNWRAP_RETURN( m, C.get<void*>( idx ) );
    Reffable* p_r = static_cast<Reffable*>( m );
    return *p_r;
  }
};

LUA_TEST_CASE( "[ext-base] cpp-owned" ) {
  maybe<MyCppOwned> m;

  SECTION( "nothing" ) {
    push( L, std::move( m ) );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::userdata );
    auto const p = get<maybe<MyCppOwned>&>( L, -1 );
    REQUIRE( p.has_value() );
    REQUIRE( !p->has_value() );
    C.pop();
  }

  SECTION( "something" ) {
    m.emplace().n = 5;
    push( L, std::move( m ) );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::userdata );
    auto const p = get<maybe<MyCppOwned>&>( L, -1 );
    REQUIRE( p.has_value() );
    REQUIRE( p->has_value() );
    REQUIRE( p->value().n == 5 );
    C.pop();
  }
}

LUA_TEST_CASE( "[ext-base] lua-owned" ) {
  maybe<MyLuaOwned> m;

  SECTION( "nothing" ) {
    push( L, std::move( m ) );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::userdata );
    // Try non-const ref.
    auto const p = get<maybe<MyLuaOwned>&>( L, -1 );
    REQUIRE( p.has_value() );
    REQUIRE( !p->has_value() );
    C.pop();
  }

  SECTION( "something" ) {
    m.emplace().n = 5;
    push( L, std::move( m ) );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::userdata );
    // Try const ref.
    auto const p = get<maybe<MyLuaOwned> const&>( L, -1 );
    REQUIRE( p.has_value() );
    REQUIRE( p->has_value() );
    REQUIRE( p->value().n == 5 );
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
    lua_expect<maybe<int>> m = get<maybe<int>>( L, -1 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == nothing );
    C.pop();
  }

  SECTION( "something" ) {
    push( L, 9 );
    lua_expect<maybe<int>> m = get<maybe<int>>( L, -1 );
    REQUIRE( m.has_value() );
    REQUIRE( m->has_value() );
    REQUIRE( **m == 9 );
    C.pop();
  }
}

LUA_TEST_CASE( "[ext-base] maybe ref" ) {
  Reffable r;
  maybe<Reffable&> m = r;

  push( L, m );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::lightuserdata );
  lua_expect<Reffable&> const m2 = get<Reffable&>( L, -1 );
  REQUIRE( m2.has_value() );
  REQUIRE( m2->x == 9 );

  C.pop();
}

} // namespace
} // namespace lua
