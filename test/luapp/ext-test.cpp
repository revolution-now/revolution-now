/****************************************************************
**ext.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-15.
*
* Description: Unit tests for the src/luapp/ext.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/ext.hpp"

// Testing
#include "test/luapp/common.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace my_ns {

using ::lua::lua_expect;

struct SomethingInternal {
  using luapp_internal = void;
};

struct Point {
  int x = 0;
  int y = 0;

  bool operator<=>( Point const& ) const = default;

  friend void lua_push( lua::cthread L, Point const& p ) {
    lua::c_api C( L );
    C.newtable();
    C.push( p.x );
    C.setfield( -2, "x" );
    C.push( p.y );
    C.setfield( -2, "y" );
  }

  friend lua_expect<Point> lua_get( lua::cthread L, int idx,
                                    lua::tag<Point> ) {
    lua::c_api C( L );
    if( C.type_of( idx ) != lua::type::table )
      // This tests that we can report custom error messages.
      return lua::unexpected{ .msg = "wrong type" };
    C.getfield( idx, "x" );
    UNWRAP_RETURN( x, C.get<int>( -1 ) );
    C.pop();
    C.getfield( idx, "y" );
    UNWRAP_RETURN( y, C.get<int>( -1 ) );
    C.pop();
    return Point{ .x = x, .y = y };
  }
};

// This type gets pushed as two Lua values.
template<typename First, typename Second>
struct Pair {
  First first   = {};
  Second second = {};

  bool operator<=>( Pair const& ) const = default;
};

struct NonPushableNonGettable {};

struct Both {
  friend void lua_push( lua::cthread L, Both const& p );
  friend lua_expect<Both> lua_get( lua::cthread L, int idx,
                                   lua::tag<Both> );
};

// This is not so much to test push/get on references, it is to
// test that maybe<T> can be pushed/popped when T is a reference.
struct ReffableViaAdl {
  int x = 9;

  friend void lua_push( lua::cthread L,
                        ReffableViaAdl const& r ) {
    lua::c_api C( L );
    push( L, (void*)&r );
  }

  friend lua_expect<ReffableViaAdl&> lua_get(
      lua::cthread L, int idx, lua::tag<ReffableViaAdl&> ) {
    lua::c_api C( L );
    CHECK( C.type_of( idx ) == lua::type::lightuserdata );
    UNWRAP_RETURN( m, C.get<void*>( idx ) );
    ReffableViaAdl* p_r = static_cast<ReffableViaAdl*>( m );
    return *p_r;
  }
};

struct ReffableViaTraits {
  int x = 9;
};

} // namespace my_ns

namespace lua {

template<>
struct type_traits<my_ns::ReffableViaTraits> {
  static constexpr int nvalues = 1;

  static void push( cthread L,
                    my_ns::ReffableViaTraits const& r ) {
    lua::c_api C( L );
    lua::push( L, (void*)&r );
  }

  static lua_expect<my_ns::ReffableViaTraits&> get(
      cthread L, int idx, tag<my_ns::ReffableViaTraits&> ) {
    lua::c_api C( L );
    CHECK( C.type_of( idx ) == lua::type::lightuserdata );
    UNWRAP_RETURN( m, C.get<void*>( idx ) );
    my_ns::ReffableViaTraits* p_r =
        static_cast<my_ns::ReffableViaTraits*>( m );
    return *p_r;
  }
};

template<Gettable First, Gettable Second>
struct type_traits<my_ns::Pair<First, Second>> {
  using P = my_ns::Pair<First, Second>;

  static constexpr int nvalues =
      nvalues_for<First>() + nvalues_for<Second>();

  static void push( cthread L, P const& p ) {
    lua::push( L, p.first );
    lua::push( L, p.second );
  }

  static lua_expect<P> get( cthread L, int idx, tag<P> ) {
    lua_expect<First> fst  = lua::get<First>( L, idx - 1 );
    lua_expect<Second> snd = lua::get<Second>( L, idx );
    if( !fst.has_value() || !snd.has_value() )
      return unexpected{};
    return P{ .first = *fst, .second = *snd };
  }
};

template<>
struct type_traits<my_ns::Both> {
  static constexpr int nvalues = 2;

  static void push( cthread L, my_ns::Both const& p );

  static lua_expect<my_ns::Both> get( cthread L, int idx,
                                      tag<my_ns::Both> );
};

} // namespace lua

namespace lua {
namespace {

using namespace std;

using ::base::valid;
using ::my_ns::Point;

static_assert( LuappInternal<my_ns::SomethingInternal> );

static_assert( !Gettable<my_ns::NonPushableNonGettable> );
static_assert( !Pushable<my_ns::NonPushableNonGettable> );
static_assert( !PushableViaAdl<my_ns::NonPushableNonGettable> );
static_assert(
    !PushableViaTraits<my_ns::NonPushableNonGettable> );
static_assert( !Gettable<my_ns::NonPushableNonGettable> );
static_assert( !GettableViaAdl<my_ns::NonPushableNonGettable> );
static_assert(
    !GettableViaTraits<my_ns::NonPushableNonGettable> );

static_assert( !Pushable<my_ns::NonPushableNonGettable> );
static_assert( !Gettable<my_ns::NonPushableNonGettable> );
static_assert( !Pushable<my_ns::Both> );
static_assert( PushableViaAdl<my_ns::Both> );
static_assert( PushableViaTraits<my_ns::Both> );
static_assert( !Gettable<my_ns::Both> );
static_assert( GettableViaAdl<my_ns::Both> );
static_assert( GettableViaTraits<my_ns::Both> );
static_assert( !LuappInternal<my_ns::Both> );

static_assert( !LuappInternal<my_ns::Point> );
static_assert( Gettable<my_ns::Point> );
static_assert( Pushable<my_ns::Point> );
static_assert( PushableViaAdl<my_ns::Point> );
static_assert( !PushableViaTraits<my_ns::Point> );
static_assert( Gettable<my_ns::Point> );
static_assert( GettableViaAdl<my_ns::Point> );
static_assert( !GettableViaTraits<my_ns::Point> );

static_assert( !LuappInternal<my_ns::Pair<int, int>> );
static_assert( Gettable<my_ns::Pair<int, int>> );
static_assert( Pushable<my_ns::Pair<int, int>> );
static_assert( !PushableViaAdl<my_ns::Pair<int, int>> );
static_assert( PushableViaTraits<my_ns::Pair<int, int>> );
static_assert( Gettable<my_ns::Pair<int, int>> );
static_assert( !GettableViaAdl<my_ns::Pair<int, int>> );
static_assert( GettableViaTraits<my_ns::Pair<int, int>> );

static_assert(
    !Pushable<my_ns::Pair<int, my_ns::NonPushableNonGettable>> );
static_assert(
    !PushableViaAdl<
        my_ns::Pair<int, my_ns::NonPushableNonGettable>> );
static_assert(
    !PushableViaTraits<
        my_ns::Pair<int, my_ns::NonPushableNonGettable>> );
static_assert(
    !Gettable<my_ns::Pair<int, my_ns::NonPushableNonGettable>> );
static_assert(
    !GettableViaAdl<
        my_ns::Pair<int, my_ns::NonPushableNonGettable>> );
static_assert(
    !GettableViaTraits<
        my_ns::Pair<int, my_ns::NonPushableNonGettable>> );

LUA_TEST_CASE( "[ext] Point" ) {
  C.openlibs();

  // push.
  Point p{ .x = 4, .y = 5 };
  push( L, p );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::table );
  C.setglobal( "my_point" );
  REQUIRE( C.stack_size() == 0 );
  REQUIRE( st["my_point"]["x"] == 4 );
  REQUIRE( st["my_point"]["y"] == 5 );

  any a = as<any>( L, Point{ .x = 1, .y = 2 } );
  REQUIRE( as<table>( a )["x"] == 1 );
  REQUIRE( as<table>( a )["y"] == 2 );

  // get.
  table lua_point = st.table.create();
  lua_point["x"]  = 7;
  lua_point["y"]  = 8;
  push( L, lua_point );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::table );
  auto const m = get<Point>( L, -1 );
  REQUIRE( m.has_value() );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
  REQUIRE( m->x == 7 );
  REQUIRE( m->y == 8 );
}

LUA_TEST_CASE( "[ext] Pair" ) {
  C.openlibs();

  using my_ns::Pair;

  using P = Pair<Pair<string, int>, double>;
  static_assert( Pushable<P> );
  static_assert( traits_for<P>::nvalues == 3 );

  P p{ { "hello", 42 }, 7.7 };

  // push.
  push( L, p );
  REQUIRE( C.stack_size() == 3 );
  REQUIRE( C.type_of( -1 ) == type::number );
  REQUIRE( C.get<double>( -1 ) == 7.7 );
  REQUIRE( C.type_of( -2 ) == type::number );
  REQUIRE( C.get<int>( -2 ) == 42 );
  REQUIRE( C.type_of( -3 ) == type::string );
  REQUIRE( C.get<string>( -3 ) == "hello" );
  C.pop( 3 );
  REQUIRE( C.stack_size() == 0 );

  // bad get.
  REQUIRE( C.dostring( "return 'world', 'no', 9.3" ) == valid );
  REQUIRE( C.stack_size() == 3 );
  REQUIRE( get<P>( L, -1 ) == unexpected{} );
  C.pop( 3 );
  REQUIRE( C.stack_size() == 0 );

  // good get.
  REQUIRE( C.dostring( "return 'world', 3, 9.3" ) == valid );
  REQUIRE( C.stack_size() == 3 );
  REQUIRE( get<P>( L, -1 ) == P{ { "world", 3 }, 9.3 } );
  C.pop( 3 );
  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[ext-base] ref via adl" ) {
  my_ns::ReffableViaAdl r;

  push( L, r );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::lightuserdata );
  auto const m2 = get<my_ns::ReffableViaAdl&>( L, -1 );
  REQUIRE( m2.has_value() );
  REQUIRE( m2->x == 9 );

  C.pop();
}

LUA_TEST_CASE( "[ext-base] ref via traits" ) {
  my_ns::ReffableViaTraits r;

  push( L, r );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::lightuserdata );
  auto const m2 = get<my_ns::ReffableViaTraits&>( L, -1 );
  REQUIRE( m2.has_value() );
  REQUIRE( m2->x == 9 );

  C.pop();
}

LUA_TEST_CASE( "[ext-base] get_or_luaerr" ) {
  auto _G = st.table.global();

  _G["fn"] = [&]( ::lua_State* const L ) {
    auto const _ = get_or_luaerr<rfunction>( L, -1 );
    return 1;
  };

  char const* const err =
      "ext-test.cpp:326:error: failed to convert Lua type "
      "`number' to native type `lua::rfunction'.\n"
      "stack traceback:\n"
      "\t[C]: in ?";

  REQUIRE( _G["fn"].pcall( 7 ) == unexpected{ .msg = err } );
}

LUA_TEST_CASE( "[ext-base] get_or_luaerr reason" ) {
  auto _G = st.table.global();

  _G["fn"] = [&]( ::lua_State* const L ) {
    auto const _ = get_or_luaerr<Point>( L, -1 );
    return 1;
  };

  char const* const err =
      "ext-test.cpp:343:error: failed to convert Lua type "
      "`number' to native type `my_ns::Point': wrong type\n"
      "stack traceback:\n"
      "\t[C]: in ?";

  REQUIRE( _G["fn"].pcall( 7 ) == unexpected{ .msg = err } );
}

} // namespace
} // namespace lua
