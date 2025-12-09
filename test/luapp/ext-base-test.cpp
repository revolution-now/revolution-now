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
LUA_USERDATA_TRAITS( MyCppOwned, owned_by_cpp ){};

static void define_usertype_for( state& st, tag<MyCppOwned> ) {
  using U = MyCppOwned;
  auto u  = st.usertype.create<U>();
  u["n"]  = &U::n;
}

namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::base::valid;

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

LUA_TEST_CASE( "[ext-base] maybe<cpp-owned>" ) {
  maybe<MyCppOwned> m;

  SECTION( "nothing" ) {
    push( L, m );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::userdata );
    auto const p = get<maybe<MyCppOwned>&>( L, -1 );
    REQUIRE( p.has_value() );
    REQUIRE( !p->has_value() );
    C.pop();
  }

  SECTION( "something" ) {
    m.emplace().n = 5;
    push( L, m );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::userdata );
    auto const p = get<maybe<MyCppOwned>&>( L, -1 );
    REQUIRE( p.has_value() );
    REQUIRE( p->has_value() );
    REQUIRE( p->value().n == 5 );
    C.pop();
  }
}

LUA_TEST_CASE( "[ext-base] API for maybe<cpp-owned>" ) {
  st.lib.open_all();
  define_usertype_for( st, tag<MyCppOwned>{} );
  define_usertype_for( st, tag<maybe<MyCppOwned>>{} );

  auto constexpr script   = R"lua(
    local m = ...
    assert( m )
    assert( not m:has_value() )
    assert( m:value() == nil )
    assert( not pcall( function()
      return m.n
    end ) )
    assert( not pcall( function()
      m.n = 5
    end ) )
    local o = m:emplace()
    o.n = 7
    assert( m:value().n == 7 )
    m.n = 5
    assert( o.n == 5 )
    assert( m:value().n == 5 )
    return 42
  )lua";
  lua::rfunction const fn = st.script.load( script );

  maybe<MyCppOwned> m;

  REQUIRE( fn.pcall<int>( m ) == 42 );

  REQUIRE( m.has_value() );
  REQUIRE( m->n == 5 );
}

LUA_TEST_CASE( "[ext-base] maybe<lua-owned>" ) {
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
