/****************************************************************
**ext-std.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-26.
*
* Description: Unit tests for the src/luapp/ext-std.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/ext-std.hpp"

// luapp
#include "src/luapp/call.hpp"
#include "src/luapp/types.hpp"

// Testing
#include "test/luapp/common.hpp"

// base
#include "base/meta.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

using namespace std;

namespace lua {

struct MyUserdata {
  int n = 5;

  bool operator==( MyUserdata const& ) const = default;
};

LUA_USERDATA_TRAITS( MyUserdata, owned_by_cpp ){};

} // namespace lua

DEFINE_FORMAT( lua::MyUserdata, "MyUserdata{{n={}}}", o.n );

namespace lua {
namespace {

LUA_TEST_CASE( "[ext-std] tuple" ) {
  SECTION( "single" ) {
    st.script.run( R"(
      function foo( n )
        return n, 'hello', 7.7
      end
    )" );

    auto t = st["foo"].call<tuple<int>>( 42 );
    static_assert( is_same_v<decltype( t ), tuple<int>> );

    REQUIRE( t == tuple{ 42 } );
  }
  SECTION( "double" ) {
    st.script.run( R"(
      function foo( n )
        return n, 'hello'
      end
    )" );

    auto t = st["foo"].call<tuple<int, string>>( 42 );
    static_assert(
        is_same_v<decltype( t ), tuple<int, string>> );

    REQUIRE( t == tuple{ 42, "hello" } );
  }
  SECTION( "many" ) {
    st.script.run( R"(
      function foo( n )
        return n, 'hello', 7.7
      end
    )" );

    auto t = st["foo"].call<tuple<int, string, double>>( 42 );
    static_assert(
        is_same_v<decltype( t ), tuple<int, string, double>> );

    REQUIRE( t == tuple{ 42, "hello", 7.7 } );
  }
  SECTION( "integers" ) {
    st.script.run( R"(
      function foo()
        return 1, 2, 3, 4, 5, 6
      end
    )" );

    auto t =
        st["foo"].call<tuple<int, int, int, int, int, int>>();
    static_assert(
        is_same_v<decltype( t ),
                  tuple<int, int, int, int, int, int>> );

    REQUIRE( t == tuple<int, int, int, int, int, int>{
                      1, 2, 3, 4, 5, 6 } );
  }
  SECTION( "with userdata" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"(
      function foo( n )
        return n, mud
      end
    )" );

    auto t = st["foo"].call<tuple<int, MyUserdata&>>( 42 );
    static_assert(
        is_same_v<decltype( t ), tuple<int, MyUserdata&>> );

    REQUIRE( t == tuple<int, MyUserdata&>{ 42, mud } );
  }
  SECTION( "not enough" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"(
      function foo( n )
        return n
      end
    )" );

    char const* err =
        "native code expected type `std::tuple<int, "
        "lua::MyUserdata&>' as a return value (which requires 2 "
        "Lua values), but the values returned by Lua were not "
        "convertible to that native type.  The Lua values "
        "received were: [nil, number].";

    REQUIRE( st["foo"].pcall<tuple<int, MyUserdata&>>( 42 ) ==
             lua_unexpected<tuple<int, MyUserdata&>>( err ) );
  }
  SECTION( "not enough but ok" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"(
      function foo( n )
        return n
      end
    )" );

    any any_nil = st["?"];
    REQUIRE( st["foo"].pcall<tuple<int, any>>( 42 ) ==
             tuple<int, any>( 42, any_nil ) );
  }
  SECTION( "too many" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"(
      function foo( n )
        return n, mud, 5, 6
      end
    )" );

    auto t = st["foo"].call<tuple<int, MyUserdata const&>>( 43 );
    mud.n  = 7;
    REQUIRE( std::get<0>( t ) == 43 );
    REQUIRE( std::get<1>( t ) == MyUserdata{ 7 } );
  }
}

LUA_TEST_CASE( "[ext-std] pair" ) {
  SECTION( "double" ) {
    st.script.run( R"(
      function foo( n )
        return n, 'hello'
      end
    )" );

    auto t = st["foo"].call<pair<int, string>>( 42 );
    static_assert( is_same_v<decltype( t ), pair<int, string>> );

    REQUIRE( t == pair<int, string>{ 42, "hello" } );
  }
  SECTION( "integers" ) {
    st.script.run( R"(
      function foo()
        return 1, 2
      end
    )" );

    auto t = st["foo"].call<pair<int, int>>();
    static_assert( is_same_v<decltype( t ), pair<int, int>> );

    REQUIRE( t == pair<int, int>{ 1, 2 } );
  }
  SECTION( "with userdata" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"(
      function foo( n )
        return n, mud
      end
    )" );

    auto t = st["foo"].call<pair<int, MyUserdata&>>( 42 );
    static_assert(
        is_same_v<decltype( t ), pair<int, MyUserdata&>> );

    REQUIRE( t == pair<int, MyUserdata&>{ 42, mud } );
  }
  SECTION( "not enough" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"(
      function foo( n )
        return n
      end
    )" );

    char const* err =
        "native code expected type `std::pair<int, "
        "lua::MyUserdata&>' as a return value (which requires 2 "
        "Lua values), but the values returned by Lua were not "
        "convertible to that native type.  The Lua values "
        "received were: [nil, number].";

    REQUIRE( st["foo"].pcall<pair<int, MyUserdata&>>( 42 ) ==
             lua_unexpected<pair<int, MyUserdata&>>( err ) );
  }
  SECTION( "not enough but ok" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"(
      function foo( n )
        return n
      end
    )" );

    any any_nil = st["?"];
    REQUIRE( st["foo"].pcall<pair<int, any>>( 42 ) ==
             pair<int, any>( 42, any_nil ) );
  }
  SECTION( "too many" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"(
      function foo( n )
        return n, mud, 5, 6
      end
    )" );

    auto t = st["foo"].call<pair<int, MyUserdata const&>>( 43 );
    mud.n  = 7;
    REQUIRE( std::get<0>( t ) == 43 );
    REQUIRE( std::get<1>( t ) == MyUserdata{ 7 } );
  }
}

} // namespace
} // namespace lua
