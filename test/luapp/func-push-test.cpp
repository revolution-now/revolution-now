/****************************************************************
**func-push.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-14.
*
* Description: Unit tests for the src/luapp/func-push.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/func-push.hpp"

// Testing
#include "test/luapp/common.hpp"
#include "test/monitoring-types.hpp"

// luapp
#include "src/luapp/as.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace lua {
namespace {

using namespace std;

using ::base::valid;
using ::Catch::Matches;
using ::testing::monitoring_types::Tracker;

/****************************************************************
** Function props.
*****************************************************************/
namespace func {

struct A {};

[[maybe_unused]] int luac_stateless_foo( lua_State* ) {
  return 0;
}
[[maybe_unused]] int not_luac_foo( lua_State& ) { return 0; }

// lua-c-extension-function.
struct StatelessLCEF {
  [[maybe_unused]] static int fun( lua_State* ) { return 0; }
};
struct LCEF {
  [[maybe_unused]] int operator()( lua_State* ) const {
    return 0;
  }
  int x;
};
struct NonLCEF {
  [[maybe_unused]] int operator()( lua_State*, int ) const {
    return 0;
  }
  int x;
};

[[maybe_unused]] auto c_ext_lambda = []( lua_State* ) -> int {
  return 0;
};
[[maybe_unused]] auto c_ext_lambda_capture =
    [x = 1.0]( lua_State* ) -> int {
  (void)x;
  return 0;
};

} // namespace func

/****************************************************************
** LuaCExtensionFunction
*****************************************************************/
static_assert(
    LuaCExtensionFunction<decltype( func::c_ext_lambda )> );
static_assert( LuaCExtensionFunction<
               decltype( func::c_ext_lambda_capture )> );

static_assert( !LuaCExtensionFunction<int> );
static_assert( !LuaCExtensionFunction<int ( * )()> );
static_assert(
    !LuaCExtensionFunction<void ( * )( lua_State* )> );
static_assert(
    !LuaCExtensionFunction<char ( * )( lua_State* )> );
static_assert( !LuaCExtensionFunction<int( void* )> );
static_assert( !LuaCExtensionFunction<int( lua_State const* )> );

static_assert( LuaCExtensionFunction<int ( * )( lua_State* )> );
static_assert( LuaCExtensionFunction<int ( & )( lua_State* )> );
static_assert( LuaCExtensionFunction<int( lua_State* ) const> );
static_assert( LuaCExtensionFunction<int( lua_State* ) const&> );
static_assert(
    LuaCExtensionFunction<int( lua_State* ) const&&> );
static_assert( LuaCExtensionFunction<int( lua_State* ) &> );
static_assert( LuaCExtensionFunction<int( lua_State* ) &&> );

static_assert( LuaCExtensionFunction<decltype( func::LCEF{} )> );
static_assert(
    !LuaCExtensionFunction<decltype( func::NonLCEF{} )> );
static_assert( LuaCExtensionFunction<
               decltype( func::StatelessLCEF::fun )> );

static_assert( LuaCExtensionFunction<
               decltype( func::luac_stateless_foo )> );
static_assert(
    !LuaCExtensionFunction<decltype( func::not_luac_foo )> );

/****************************************************************
** {Stateless,Stateful}LuaCExtensionFunction
*****************************************************************/
static_assert( StatelessLuaCExtensionFunction<
               decltype( func::c_ext_lambda )> );
static_assert( !StatelessLuaCExtensionFunction<
               decltype( func::c_ext_lambda_capture )> );

static_assert( !StatelessLuaCExtensionFunction<int> );
static_assert( !StatelessLuaCExtensionFunction<int ( * )()> );
static_assert(
    !StatelessLuaCExtensionFunction<void ( * )( lua_State* )> );
static_assert(
    !StatelessLuaCExtensionFunction<char ( * )( lua_State* )> );
static_assert( !StatelessLuaCExtensionFunction<int( void* )> );
static_assert(
    !StatelessLuaCExtensionFunction<int( lua_State const* )> );

static_assert(
    StatelessLuaCExtensionFunction<int ( * )( lua_State* )> );
static_assert(
    StatelessLuaCExtensionFunction<int ( & )( lua_State* )> );
static_assert(
    !StatelessLuaCExtensionFunction<int( lua_State* ) const> );
static_assert(
    !StatelessLuaCExtensionFunction<int( lua_State* ) const&> );
static_assert(
    !StatelessLuaCExtensionFunction<int( lua_State* ) const&&> );
static_assert(
    !StatelessLuaCExtensionFunction<int( lua_State* ) &> );
static_assert(
    !StatelessLuaCExtensionFunction<int( lua_State* ) &&> );

static_assert(
    !StatelessLuaCExtensionFunction<decltype( func::LCEF{} )> );
static_assert( !StatelessLuaCExtensionFunction<
               decltype( func::NonLCEF{} )> );
static_assert( StatelessLuaCExtensionFunction<
               decltype( func::StatelessLCEF::fun )> );

static_assert( StatelessLuaCExtensionFunction<
               decltype( func::luac_stateless_foo )> );
static_assert( !StatelessLuaCExtensionFunction<
               decltype( func::not_luac_foo )> );

static_assert( !StatefulLuaCExtensionFunction<
               decltype( func::c_ext_lambda )> );
static_assert( StatefulLuaCExtensionFunction<
               decltype( func::c_ext_lambda_capture )> );

static_assert( !StatefulLuaCExtensionFunction<int> );
static_assert( !StatefulLuaCExtensionFunction<int ( * )()> );
static_assert(
    !StatefulLuaCExtensionFunction<void ( * )( lua_State* )> );
static_assert(
    !StatefulLuaCExtensionFunction<char ( * )( lua_State* )> );
static_assert( !StatefulLuaCExtensionFunction<int( void* )> );
static_assert(
    !StatefulLuaCExtensionFunction<int( lua_State const* )> );

static_assert(
    !StatefulLuaCExtensionFunction<int ( * )( lua_State* )> );
static_assert(
    !StatefulLuaCExtensionFunction<int ( & )( lua_State* )> );
static_assert(
    StatefulLuaCExtensionFunction<int( lua_State* ) const> );
static_assert(
    StatefulLuaCExtensionFunction<int( lua_State* ) const&> );
static_assert(
    StatefulLuaCExtensionFunction<int( lua_State* ) const&&> );
static_assert(
    StatefulLuaCExtensionFunction<int( lua_State* ) &> );
static_assert(
    StatefulLuaCExtensionFunction<int( lua_State* ) &&> );
static_assert( !StatefulLuaCExtensionFunction<int( int* ) &&> );

static_assert(
    StatefulLuaCExtensionFunction<decltype( func::LCEF{} )> );
static_assert( !StatefulLuaCExtensionFunction<
               decltype( func::NonLCEF{} )> );
static_assert( !StatefulLuaCExtensionFunction<
               decltype( func::StatelessLCEF::fun )> );

static_assert( !StatefulLuaCExtensionFunction<
               decltype( func::luac_stateless_foo )> );
static_assert( !StatefulLuaCExtensionFunction<
               decltype( func::not_luac_foo )> );

/****************************************************************
** push for functions.
*****************************************************************/
LUA_TEST_CASE( "[func-push] push stateless lua C function" ) {
  int ( *f )( lua_State* );
  f = +[]( lua_State* ) -> int { return 0; };
  lua_push( L, f );

  auto g = []( lua_State* ) -> int { return 0; };
  lua_push( L, g );

  C.pop( 2 );
}

LUA_TEST_CASE( "[func-push] stateless lua C function" ) {
  C.openlibs();

  SECTION( "temporary" ) {
    push( L, []( lua_State* L ) -> int {
      c_api C( L );
      UNWRAP_CHECK( n, C.get<int>( 1 ) );
      C.push( n + 1 );
      return 1;
    } );
  }

  SECTION( "move" ) {
    auto f = []( lua_State* L ) -> int {
      c_api C( L );
      UNWRAP_CHECK( n, C.get<int>( 1 ) );
      C.push( n + 1 );
      return 1;
    };
    push( L, std::move( f ) );
  }

  REQUIRE( C.stack_size() == 1 );
  C.setglobal( "add_one" );
  REQUIRE( C.stack_size() == 0 );

  REQUIRE( C.dostring( "assert( add_one( 6 ) == 7 )" ) ==
           valid );

  // Make sure that it has no upvalues.
  C.getglobal( "add_one" );
  REQUIRE( C.type_of( -1 ) == type::function );
  REQUIRE_FALSE( C.getupvalue( -1, 1 ) );
  C.pop();
}

LUA_TEST_CASE( "[func-push] stateful lua C function" ) {
  Tracker::reset();

  SECTION( "__gc metamethod is called, twice" ) {
    C.openlibs();

    push( L, [tracker = Tracker{}]( lua_State* L ) -> int {
      c_api C( L );
      UNWRAP_CHECK( n, C.get<int>( 1 ) );
      C.push( n + 1 );
      return 1;
    } );
    C.setglobal( "add_one" );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 1 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();

    REQUIRE( C.dostring( "assert( add_one( 6 ) == 7 )" ) ==
             valid );
    REQUIRE( C.dostring( "assert( add_one( 7 ) == 8 )" ) ==
             valid );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();

    // Test that the function has an up value and that the upval-
    // ue's metatable has the right name.
    C.getglobal( "add_one" );
    REQUIRE( C.type_of( -1 ) == type::function );
    REQUIRE_FALSE( C.getupvalue( -1, 2 ) );
    REQUIRE( C.getupvalue( -1, 1 ) == true );
    REQUIRE( C.type_of( -1 ) == type::userdata );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.getmetatable( -1 ) );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.getfield( -1, "__name" ) == type::string );
    REQUIRE( C.stack_size() == 4 );
    REQUIRE_THAT(
        *C.get<string>( -1 ),
        Matches( "lua::.anonymous namespace.::.anonymous "
                 "namespace.::____C_A_T_C_H____T_E_S_T____"
                 "[0-9]+::test..::.*" ) );

    C.pop( 4 );
    REQUIRE( C.stack_size() == 0 );

    // Now set a second closure.
    push( L, [tracker = Tracker{}]( lua_State* L ) -> int {
      c_api C( L );
      UNWRAP_CHECK( n, C.get<int>( 1 ) );
      C.push( n + 2 );
      return 1;
    } );
    C.setglobal( "add_two" );
    REQUIRE( Tracker::constructed == 1 );
    REQUIRE( Tracker::destructed == 1 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 1 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();

    REQUIRE( C.dostring( "assert( add_two( 6 ) == 8 )" ) ==
             valid );
    REQUIRE( C.dostring( "assert( add_two( 7 ) == 9 )" ) ==
             valid );
    REQUIRE( Tracker::constructed == 0 );
    REQUIRE( Tracker::destructed == 0 );
    REQUIRE( Tracker::copied == 0 );
    REQUIRE( Tracker::move_constructed == 0 );
    REQUIRE( Tracker::move_assigned == 0 );
    Tracker::reset();
  }

  st.free();
  // !! do not call any lua functions after this.

  // Ensure that precisely two closures get destroyed (will
  // happen when `st` goes out of scope and Lua calls the final-
  // izers on the userdatas for the two closures that we created
  // above).
  REQUIRE( Tracker::constructed == 0 );
  REQUIRE( Tracker::destructed == 2 );
  REQUIRE( Tracker::copied == 0 );
  REQUIRE( Tracker::move_constructed == 0 );
  REQUIRE( Tracker::move_assigned == 0 );
}

// This is a callable that adds some numbers, and also tracks
// when it gets destroyed. It is a bit non-trivial because we
// have to ensure that moved-from instances do not track a de-
// struction -- that is because we want to know when all in-
// stances are gone.
struct AdderTracker {
  AdderTracker( int x, int y, bool* destroyed )
    : x_( x ), y_( y ), destroyed_( destroyed ) {}
  ~AdderTracker() {
    if( destroyed_ ) *destroyed_ = true;
  }
  int operator()( lua_State* L ) const {
    c_api C( L );
    UNWRAP_CHECK( n, C.get<int>( 1 ) );
    C.push( n + 2 + x_ + y_ );
    return 1;
  }
  AdderTracker( AdderTracker const& )            = delete;
  AdderTracker& operator=( AdderTracker const& ) = delete;
  AdderTracker( AdderTracker&& rhs ) {
    x_         = rhs.x_;
    y_         = rhs.y_;
    destroyed_ = exchange( rhs.destroyed_, nullptr );
  }

  int   x_;
  int   y_;
  bool* destroyed_;
};

LUA_TEST_CASE( "[func-push] another stateful lua C function" ) {
  C.openlibs();

  int x = 4;
  int y = 9;

  SECTION( "lambda with captures" ) {
    push( L, [=]( lua_State* L ) -> int {
      c_api C( L );
      UNWRAP_CHECK( n, C.get<int>( 1 ) );
      C.push( n + 1 + x + y );
      return 1;
    } );
    C.setglobal( "add_some" );

    REQUIRE( C.dostring( "assert( add_some( 6 ) == 20 )" ) ==
             valid );
  }

  SECTION( "class object with captures" ) {
    bool destroyed = false;
    {
      push( L, AdderTracker( x, y, &destroyed ) );
      C.setglobal( "add_some" );
      REQUIRE( C.dostring( "assert( add_some( 6 ) == 21 )" ) ==
               valid );
    }
    // Must do this to ensure that the AdderTracker gets de-
    // stroyed before the `destroyed` variable goes out of scope,
    // since it will try to write to it in its destructor. If we
    // didn't do this, then since the AdderTracker object is tied
    // to a global symbol ("add_some"), the Lua garbage collector
    // would not release it until the state got destroyed
    // (wherein it runs the finalizers on things), but by then
    // the `destroyed variable will have gone out of scope.
    C.push( nil );
    C.setglobal( "add_some" ); // remove reference to it.
    C.gc_collect();
    REQUIRE( destroyed );
  }
}

LUA_TEST_CASE(
    "[func-push] push_cpp_function, cpp function has upvalue" ) {
  C.openlibs();

  push_cpp_function(
      L, []( int n, string const& s, double d ) -> string {
        return fmt::format( "args: n={}, s='{}', d={}", n, s,
                            d );
      } );
  C.setglobal( "go" );

  // Make sure that it has no upvalues.
  C.getglobal( "go" );
  REQUIRE( C.type_of( -1 ) == type::function );
  REQUIRE_FALSE( C.getupvalue( -1, 2 ) );
  REQUIRE( C.getupvalue( -1, 1 ) == true );
  REQUIRE( C.type_of( -1 ) == type::userdata );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.getmetatable( -1 ) );
  REQUIRE( C.stack_size() == 3 );
  REQUIRE( C.getfield( -1, "__name" ) == type::string );
  REQUIRE( C.stack_size() == 4 );
  // Don't test the string name, because it's long and ugly.
  C.pop( 4 );
}

LUA_TEST_CASE(
    "[func-push] push_cpp_function, cpp function, trivial" ) {
  bool called = false;

  push_cpp_function( L, [&] { called = !called; } );
  C.setglobal( "go" );
  REQUIRE_FALSE( called );

  REQUIRE( C.dostring( "go()" ) == valid );
  REQUIRE( called );
  REQUIRE( C.dostring( "go()" ) == valid );
  REQUIRE_FALSE( called );
  REQUIRE( C.dostring( "go()" ) == valid );
  REQUIRE( called );
  REQUIRE( C.dostring( "go()" ) == valid );
  REQUIRE_FALSE( called );
}

LUA_TEST_CASE(
    "[func-push] push_cpp_function, cpp function, "
    "simple/bool" ) {
  bool called_with = false;

  push_cpp_function( L, [&]( bool b ) { called_with = b; } );
  C.setglobal( "go" );
  REQUIRE_FALSE( called_with );

  REQUIRE( C.dostring( "go( false )" ) == valid );
  REQUIRE_FALSE( called_with );

  REQUIRE( C.dostring( "go( false )" ) == valid );
  REQUIRE_FALSE( called_with );

  REQUIRE( C.dostring( "go( true )" ) == valid );
  REQUIRE( called_with );

  REQUIRE( C.dostring( "go( 'hello' )" ) == valid );
  REQUIRE( called_with );

  REQUIRE( C.dostring( "go( nil )" ) == valid );
  REQUIRE_FALSE( called_with );
}

LUA_TEST_CASE(
    "[func-push] push_cpp_function, cpp function, calling" ) {
  C.openlibs();

  push_cpp_function(
      L, []( int n, string const& s, double d ) -> string {
        return fmt::format( "args: n={}, s='{}', d={}", n, s,
                            d );
      } );
  C.setglobal( "go" );

  SECTION( "successful call" ) {
    REQUIRE( C.dostring( R"lua(
      local result =
        go( 6, 'hello this is a very long string', 3.5 )
      local expected =
        "args: n=6, s='hello this is a very long string', d=3.5"
      local err = tostring( result ) .. ' not equal to "' ..
                  tostring( expected ) .. '".'
      assert( result == expected, err )
    )lua" ) == valid );
  }

  SECTION( "too few args" ) {
    // clang-format off
    char const* err =
      "Native function expected 3 arguments, but received 2 from Lua.\n"
      "stack traceback:\n"
      "\t[C]: in function 'go'\n"
      "\t[string \"...\"]:2: in main chunk";
    // clang-format on

    REQUIRE( C.dostring( R"lua(
      go( 6, 'hello this is a very long string' )
    )lua" ) == lua_invalid( err ) );
  }

  SECTION( "too many args" ) {
    // clang-format off
    char const* err =
      "Native function expected 3 arguments, but received 4 from Lua.\n"
      "stack traceback:\n"
      "\t[C]: in function 'go'\n"
      "\t[string \"...\"]:2: in main chunk";
    // clang-format on

    REQUIRE( C.dostring( R"lua(
      go( 6, 'hello this is a very long string', 3.5, true )
    )lua" ) == lua_invalid( err ) );
  }

  SECTION( "wrong arg type" ) {
    // clang-format off
    char const* err =
      "Native function expected type 'double' for argument 3 "
        "(1-based), but received non-convertible type 'string' "
        "from Lua.\n"
      "stack traceback:\n"
      "\t[C]: in function 'go'\n"
      "\t[string \"...\"]:2: in main chunk";
    // clang-format on
    REQUIRE( C.dostring( R"lua(
      go( 6, 'hello this is a very long string', 'world' )
    )lua" ) == lua_invalid( err ) );
  }

  SECTION( "convertible arg types" ) {
    REQUIRE( C.dostring( R"lua(
      local result = go( '6', 1.23, '3.5' )
      local expected = "args: n=6, s='1.23', d=3.5"
      local err = tostring( result ) .. ' not equal to "' ..
                  tostring( expected ) .. '".'
      assert( result == expected, err )
    )lua" ) == valid );
  }
}

LUA_TEST_CASE(
    "[func-push] string_view and string ref params" ) {
  static_assert( !StorageGettable<string&> );
  static_assert( StorageGettable<string const&> );
  static_assert( StorageGettable<string&&> );
  static_assert( StorageGettable<string> );
  st["foo"] = []( string&& s, string const& s2,
                  string_view sv ) {
    REQUIRE( &s[2] != &sv[2] );
    REQUIRE( &s[2] != &s2[2] );
    return s + "-" + s2 + "-" + string( sv );
  };
  REQUIRE( as<string>( st["foo"]( "one", "two", "three" ) ) ==
           "one-two-three" );
}

} // namespace
} // namespace lua
