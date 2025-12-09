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
#include "src/luapp/ext-monostate.hpp"
#include "src/luapp/types.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Testing
#include "test/luapp/common.hpp"

// base
#include "base/meta.hpp"

// Must be last.
#include "test/catch-common.hpp"

using namespace std;

namespace lua {

struct MyUserdata {
  int n = 5;

  bool operator==( MyUserdata const& ) const = default;

  friend void to_str( MyUserdata const& o, std::string& out,
                      base::tag<MyUserdata> ) {
    out += fmt::format( "MyUserdata{{n={}}}", o.n );
  }
};

LUA_USERDATA_TRAITS( MyUserdata, owned_by_cpp ){};

static void define_usertype_for( state& st, tag<MyUserdata> ) {
  using U = MyUserdata;
  auto u  = st.usertype.create<U>();
  u["n"]  = &U::n;
}

namespace {

using ::Catch::Matches;

LUA_TEST_CASE( "[ext-std] std::monostate" ) {
  lua::push( L, monostate{} );
  REQUIRE( C.type_of( -1 ) == type::nil );

  REQUIRE( lua::get<monostate>( L, -1 ) == monostate{} );

  lua::push( L, 5 );
  lua::push( L, "hello" );
  REQUIRE( lua::get<monostate>( L, -1 ) == unexpected{} );
  REQUIRE( lua::get<monostate>( L, -2 ) == unexpected{} );

  C.pop( 3 );
}

LUA_TEST_CASE( "[ext-std] tuple/pair push/get" ) {
  SECTION( "tuple" ) {
    tuple<double, string, int> t{ 7.7, "hello", 9 };
    REQUIRE( C.stack_size() == 0 );
    lua::push( L, t );
    REQUIRE( C.stack_size() == 3 );
    auto m = lua::get<tuple<double, string, int>>( L, -1 );
    REQUIRE( m.has_value() );
    REQUIRE( m == t );
    C.pop( 3 );
  }
  SECTION( "pair" ) {
    pair<double, string> p{ 7.7, "hello" };
    REQUIRE( C.stack_size() == 0 );
    lua::push( L, p );
    REQUIRE( C.stack_size() == 2 );
    auto m = lua::get<pair<double, string>>( L, -1 );
    REQUIRE( m.has_value() );
    REQUIRE( m == p );
    C.pop( 2 );
  }
}

LUA_TEST_CASE( "[ext-std] tuple" ) {
  SECTION( "single" ) {
    st.script.run( R"lua(
      function foo( n )
        return n, 'hello', 7.7
      end
    )lua" );

    auto t = st["foo"].call<tuple<int>>( 42 );
    static_assert( is_same_v<decltype( t ), tuple<int>> );

    REQUIRE( ( t == tuple{ 42 } ) );
  }
  SECTION( "double" ) {
    st.script.run( R"lua(
      function foo( n )
        return n, 'hello'
      end
    )lua" );

    auto t = st["foo"].call<tuple<int, string>>( 42 );
    static_assert(
        is_same_v<decltype( t ), tuple<int, string>> );

    REQUIRE( ( t == tuple{ 42, "hello" } ) );
  }
  SECTION( "many" ) {
    st.script.run( R"lua(
      function foo( n )
        return n, 'hello', 7.7
      end
    )lua" );

    auto t = st["foo"].call<tuple<int, string, double>>( 42 );
    static_assert(
        is_same_v<decltype( t ), tuple<int, string, double>> );

    REQUIRE( ( t == tuple{ 42, "hello", 7.7 } ) );
  }
  SECTION( "integers" ) {
    st.script.run( R"lua(
      function foo()
        return 1, 2, 3, 4, 5, 6
      end
    )lua" );

    auto t =
        st["foo"].call<tuple<int, int, int, int, int, int>>();
    static_assert(
        is_same_v<decltype( t ),
                  tuple<int, int, int, int, int, int>> );

    REQUIRE( ( t == tuple<int, int, int, int, int, int>{
                      1, 2, 3, 4, 5, 6 } ) );
  }
  SECTION( "with userdata" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"lua(
      function foo( n )
        return n, mud
      end
    )lua" );

    auto t = st["foo"].call<tuple<int, MyUserdata&>>( 42 );
    static_assert(
        is_same_v<decltype( t ), tuple<int, MyUserdata&>> );

    REQUIRE( ( t == tuple<int, MyUserdata&>{ 42, mud } ) );
  }
  SECTION( "not enough" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"lua(
      function foo( n )
        return n
      end
    )lua" );

    auto res = st["foo"].pcall<tuple<int, MyUserdata&>>( 42 );
    REQUIRE( !res.has_value() );
    REQUIRE_THAT(
        res.error().msg,
        Matches(
            "native code expected type `std::.*tuple\\<int, "
            "lua::MyUserdata&\\>' as a return value \\(which "
            "requires 2 Lua values\\), but the values returned "
            "by Lua were not convertible to that native type.  "
            "The Lua values received were: \\[nil, "
            "number\\]." ) );
  }
  SECTION( "not enough but ok" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"lua(
      function foo( n )
        return n
      end
    )lua" );

    any any_nil = st["?"];
    REQUIRE( st["foo"].pcall<tuple<int, any>>( 42 ) ==
             tuple<int, any>( 42, any_nil ) );
  }
  SECTION( "too many" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"lua(
      function foo( n )
        return n, mud, 5, 6
      end
    )lua" );

    auto t = st["foo"].call<tuple<int, MyUserdata const&>>( 43 );
    mud.n  = 7;
    REQUIRE( std::get<0>( t ) == 43 );
    REQUIRE( std::get<1>( t ) == MyUserdata{ 7 } );
  }
}

LUA_TEST_CASE( "[ext-std] pair" ) {
  SECTION( "double" ) {
    st.script.run( R"lua(
      function foo( n )
        return n, 'hello'
      end
    )lua" );

    auto t = st["foo"].call<pair<int, string>>( 42 );
    static_assert( is_same_v<decltype( t ), pair<int, string>> );

    REQUIRE( t == pair<int, string>{ 42, "hello" } );
  }
  SECTION( "integers" ) {
    st.script.run( R"lua(
      function foo()
        return 1, 2
      end
    )lua" );

    auto t = st["foo"].call<pair<int, int>>();
    static_assert( is_same_v<decltype( t ), pair<int, int>> );

    REQUIRE( t == pair<int, int>{ 1, 2 } );
  }
  SECTION( "with userdata" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"lua(
      function foo( n )
        return n, mud
      end
    )lua" );

    auto t = st["foo"].call<pair<int, MyUserdata&>>( 42 );
    static_assert(
        is_same_v<decltype( t ), pair<int, MyUserdata&>> );

    REQUIRE( t == pair<int, MyUserdata&>{ 42, mud } );
  }
  SECTION( "not enough" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"lua(
      function foo( n )
        return n
      end
    )lua" );

    auto res = st["foo"].pcall<pair<int, MyUserdata&>>( 42 );
    REQUIRE( !res.has_value() );
    REQUIRE_THAT(
        res.error().msg,
        Matches( "native code expected type `std::.*pair\\<int, "
                 "lua::MyUserdata&\\>' as a return value "
                 "\\(which requires 2 Lua values\\), but the "
                 "values returned by Lua were not convertible "
                 "to that native type.  The Lua values received "
                 "were: \\[nil, number\\]." ) );
  }
  SECTION( "not enough but ok" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"lua(
      function foo( n )
        return n
      end
    )lua" );

    any any_nil = st["?"];
    REQUIRE( st["foo"].pcall<pair<int, any>>( 42 ) ==
             pair<int, any>( 42, any_nil ) );
  }
  SECTION( "too many" ) {
    MyUserdata mud{ .n = 9 };
    st["mud"] = mud;

    st.script.run( R"lua(
      function foo( n )
        return n, mud, 5, 6
      end
    )lua" );

    auto t = st["foo"].call<pair<int, MyUserdata const&>>( 43 );
    mud.n  = 7;
    REQUIRE( std::get<0>( t ) == 43 );
    REQUIRE( std::get<1>( t ) == MyUserdata{ 7 } );
  }
}

LUA_TEST_CASE( "[ext-std] std::map API" ) {
  st.lib.open_all();

  SECTION( "int value" ) {
    using M = std::map<string, int>;
    define_usertype_for( st, tag<M>{} );

    auto constexpr script   = R"lua(
      local m = ...
      assert( m )
      assert( m.hello == 5 )
      assert( m.world == 7 )
      assert( m.xyz == nil )
      assert( m:size() == 2 )
      m:clear()
      assert( m:size() == 0 )
      m.new = 99
      m.hello = 5
      return 42
    )lua";
    lua::rfunction const fn = st.script.load( script );

    M m;
    m["hello"] = 5;
    m["world"] = 7;

    REQUIRE( fn.pcall<int>( m ) == 42 );
    REQUIRE( m == M{ { "hello", 5 }, { "new", 99 } } );
  }

  SECTION( "userdata value" ) {
    using M = std::map<string, MyUserdata>;
    define_usertype_for( st, tag<MyUserdata>{} );
    define_usertype_for( st, tag<M>{} );

    auto constexpr script   = R"lua(
      local m = ...
      assert( m )
      assert( m.hello.n == 5 )
      assert( m.world.n == 9 )
      assert( m.xyz == nil )
      assert( m:size() == 2 )
      m:clear()
      assert( m:size() == 0 )
      local new = m:make( 'new' )
      new.n = 4
      local hello = m:make( 'hello' )
      hello.n = 6
      return 42
    )lua";
    lua::rfunction const fn = st.script.load( script );

    M m;
    m["hello"] = MyUserdata{};
    m["world"] = MyUserdata{ .n = 9 };

    REQUIRE( fn.pcall<int>( m ) == 42 );
    REQUIRE( m == M{ { "hello", MyUserdata{ .n = 6 } },
                     { "new", MyUserdata{ .n = 4 } } } );
  }
}

LUA_TEST_CASE( "[ext-std] std::unordered_map API" ) {
  st.lib.open_all();

  SECTION( "int value" ) {
    using M = std::unordered_map<string, int>;
    define_usertype_for( st, tag<M>{} );

    auto constexpr script   = R"lua(
      local m = ...
      assert( m )
      assert( m.hello == 5 )
      assert( m.world == 7 )
      assert( m.xyz == nil )
      assert( m:size() == 2 )
      m:clear()
      assert( m:size() == 0 )
      m.new = 99
      m.hello = 5
      return 42
    )lua";
    lua::rfunction const fn = st.script.load( script );

    M m;
    m["hello"] = 5;
    m["world"] = 7;

    REQUIRE( fn.pcall<int>( m ) == 42 );
    REQUIRE( m == M{ { "hello", 5 }, { "new", 99 } } );
  }

  SECTION( "userdata value" ) {
    using M = std::unordered_map<string, MyUserdata>;
    define_usertype_for( st, tag<MyUserdata>{} );
    define_usertype_for( st, tag<M>{} );

    auto constexpr script   = R"lua(
      local m = ...
      assert( m )
      assert( m.hello.n == 5 )
      assert( m.world.n == 9 )
      assert( m.xyz == nil )
      assert( m:size() == 2 )
      m:clear()
      assert( m:size() == 0 )
      local new = m:make( 'new' )
      new.n = 4
      local hello = m:make( 'hello' )
      hello.n = 6
      return 42
    )lua";
    lua::rfunction const fn = st.script.load( script );

    M m;
    m["hello"] = MyUserdata{};
    m["world"] = MyUserdata{ .n = 9 };

    REQUIRE( fn.pcall<int>( m ) == 42 );
    REQUIRE( m == M{ { "hello", MyUserdata{ .n = 6 } },
                     { "new", MyUserdata{ .n = 4 } } } );
  }
}

LUA_TEST_CASE( "[ext-std] std::vector API" ) {
  st.lib.open_all();

  SECTION( "int value" ) {
    using V = std::vector<int>;
    define_usertype_for( st, tag<V>{} );

    auto constexpr script   = R"lua(
      local m = ...
      assert( m )
      assert( m[1] == 5 )
      assert( m[2] == 7 )
      assert( not pcall( function()
        return m.xyz
      end ) )
      assert( m:size() == 2 )
      m:clear()
      assert( m:size() == 0 )
      m:add()
      m[1] = 99
      m:add()
      m[2] = 5
      return 42
    )lua";
    lua::rfunction const fn = st.script.load( script );

    V m;
    m.push_back( 5 );
    m.push_back( 7 );

    REQUIRE( fn.pcall<int>( m ) == 42 );
    REQUIRE( m == V{ 99, 5 } );
  }

  SECTION( "userdata value" ) {
    using V = std::vector<MyUserdata>;
    define_usertype_for( st, tag<MyUserdata>{} );
    define_usertype_for( st, tag<V>{} );

    auto constexpr script   = R"lua(
      local m = ...
      assert( m )
      assert( m[1].n == 5 )
      assert( m[2].n == 9 )
      assert( not pcall( function()
        return m.xyz
      end ) )
      assert( m:size() == 2 )
      m:clear()
      assert( m:size() == 0 )
      m:add()
      m[1].n = 6
      m:add()
      m[2].n = 4
      m:add()
      m[3].n = 0
      return 42
    )lua";
    lua::rfunction const fn = st.script.load( script );

    V m;
    m.push_back( MyUserdata{} );
    m.push_back( MyUserdata{ .n = 9 } );

    REQUIRE( fn.pcall<int>( m ) == 42 );
    REQUIRE( m == V{ MyUserdata{ .n = 6 }, MyUserdata{ .n = 4 },
                     MyUserdata{ .n = 0 } } );
  }
}

LUA_TEST_CASE( "[ext-std] std::array API" ) {
  st.lib.open_all();

  SECTION( "int value" ) {
    using V = std::array<int, 2>;
    define_usertype_for( st, tag<V>{} );

    auto constexpr script   = R"lua(
      local m = ...
      assert( m )
      assert( m[1] == 5 )
      assert( m[2] == 7 )
      assert( not pcall( function()
        return m.xyz
      end ) )
      assert( m:size() == 2 )
      m[1] = 99
      m[2] = 5
      return 42
    )lua";
    lua::rfunction const fn = st.script.load( script );

    V m;
    m[0] = 5;
    m[1] = 7;

    REQUIRE( fn.pcall<int>( m ) == 42 );
    REQUIRE( m == V{ 99, 5 } );
  }

  SECTION( "userdata value" ) {
    using V = std::array<MyUserdata, 2>;
    define_usertype_for( st, tag<MyUserdata>{} );
    define_usertype_for( st, tag<V>{} );

    auto constexpr script   = R"lua(
      local m = ...
      assert( m )
      assert( m[1].n == 5 )
      assert( m[2].n == 9 )
      assert( not pcall( function()
        return m.xyz
      end ) )
      assert( m:size() == 2 )
      m[1].n = 6
      m[2].n = 4
      return 42
    )lua";
    lua::rfunction const fn = st.script.load( script );

    V m;
    m[0] = MyUserdata{};
    m[1] = MyUserdata{ .n = 9 };

    REQUIRE( fn.pcall<int>( m ) == 42 );
    REQUIRE( m ==
             V{ MyUserdata{ .n = 6 }, MyUserdata{ .n = 4 } } );
  }
}

LUA_TEST_CASE( "[ext-std] std::deque API" ) {
  st.lib.open_all();

  using V = std::deque<int>;
  define_usertype_for( st, tag<V>{} );

  auto constexpr script   = R"lua(
    local m = ...
    assert( m )
    assert( m:size() == 2 )
    return 42
  )lua";
  lua::rfunction const fn = st.script.load( script );

  V m;
  m.push_back( 5 );
  m.push_back( 7 );

  REQUIRE( fn.pcall<int>( m ) == 42 );
  REQUIRE( m == V{ 5, 7 } );
}

} // namespace
} // namespace lua
