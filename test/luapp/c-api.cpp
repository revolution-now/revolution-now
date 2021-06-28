/****************************************************************
**c-api.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-27.
*
* Description: Unit tests for the src/luapp/c-api.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/c-api.hpp"

// Testing
#include "test/luapp/common.hpp"

// Lua
#include "lauxlib.h"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::base::valid;
using ::Catch::Matches;

string lua_testing_file( string const& filename ) {
  return rn::testing::data_dir() / "lua" / filename;
}

LUA_TEST_CASE( "[lua-c-api] create and destroy" ) {}

LUA_TEST_CASE( "[lua-c-api] openlibs" ) {
  REQUIRE( C.getglobal( "tostring" ) == type::nil );
  REQUIRE( C.stack_size() == 1 );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
  C.openlibs();
  REQUIRE( C.getglobal( "tostring" ) == type::function );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.enforce_type_of( -1, type::function ) == valid );
  C.pop();
}

LUA_TEST_CASE( "[lua-c-api] rotate" ) {
  C.push( true );
  C.push( "hello" );
  C.push( false );
  C.push( 3.5 );
  REQUIRE( C.stack_size() == 4 );
  REQUIRE( C.enforce_type_of( -1, type::number ) == valid );
  REQUIRE( C.enforce_type_of( -2, type::boolean ) == valid );
  REQUIRE( C.enforce_type_of( -3, type::string ) == valid );
  REQUIRE( C.enforce_type_of( -4, type::boolean ) == valid );

  C.rotate( -3, 2 );

  REQUIRE( C.stack_size() == 4 );
  REQUIRE( C.enforce_type_of( -1, type::string ) == valid );
  REQUIRE( C.enforce_type_of( -2, type::number ) == valid );
  REQUIRE( C.enforce_type_of( -3, type::boolean ) == valid );
  REQUIRE( C.enforce_type_of( -4, type::boolean ) == valid );

  C.pop( 4 );
}

LUA_TEST_CASE( "[lua-c-api] {get,set}global" ) {
  REQUIRE( C.getglobal( "xyz" ) == type::nil );
  REQUIRE( C.stack_size() == 1 );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
  C.push( 1 );
  C.setglobal( "xyz" );
  REQUIRE( C.stack_size() == 0 );
  REQUIRE( C.getglobal( "xyz" ) == type::number );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.get<int>( -1 ) == 1 );
  REQUIRE( C.get<integer>( -1 ) == 1 );
  C.pop();
}

LUA_TEST_CASE( "[lua-c-api] getglobal with __index/error" ) {
  C.openlibs();
  REQUIRE( C.dostring( R"(
    setmetatable( _G, {
      __index = function( k )
        error( 'this is an error.' )
      end
    } )
  )" ) == valid );

  // clang-format off
  char const* err =
    "[string \"...\"]:4: this is an error."                  "\n"
    "stack traceback:"                                       "\n"
    "\t[C]: in function 'error'"                             "\n"
    "\t[string \"...\"]:4: in function <[string \"...\"]:3>" "\n"
    "\t[C]: in ?";
  // clang-format on
  REQUIRE( C.getglobal_safe( "xyz" ) ==
           lua_unexpected<type>( err ) );
  REQUIRE( C.stack_size() == 0 );
  REQUIRE( C.dostring( "xyz = 1" ) == valid );
  REQUIRE( C.getglobal( "xyz" ) == type::number );
  REQUIRE( C.stack_size() == 1 );
  C.pop();
}

LUA_TEST_CASE( "[lua-c-api] setglobal with __index/error" ) {
  C.openlibs();
  REQUIRE( C.dostring( R"(
    setmetatable( _G, {
      __newindex = function( k )
        error( 'this is an error.' )
      end
    } )
  )" ) == valid );
  REQUIRE( C.getglobal( "xyz" ) == type::nil );
  REQUIRE( C.stack_size() == 1 );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
  C.push( 1 );
  // clang-format off
  char const* err =
    "[string \"...\"]:4: this is an error."                  "\n"
    "stack traceback:"                                       "\n"
    "\t[C]: in function 'error'"                             "\n"
    "\t[string \"...\"]:4: in function <[string \"...\"]:3>" "\n"
    "\t[C]: in ?";
  // clang-format on
  REQUIRE( C.setglobal_safe( "xyz" ) == lua_invalid( err ) );

  // The pinvoke will get rid of the parameter.
  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] dofile" ) {
  C.openlibs();

  SECTION( "non-existent" ) {
    REQUIRE( C.dofile( lua_testing_file( "xxx.lua" ) ) ==
             lua_invalid( "cannot open test/data/lua/xxx.lua: "
                          "No such file or directory" ) );
    REQUIRE( C.stack_size() == 0 );
  }

  SECTION( "exists" ) {
    REQUIRE( C.dofile( lua_testing_file(
                 "c-api-dofile.lua" ) ) == valid );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.enforce_type_of( -1, type::table ) == valid );
    C.setglobal( "my_module" );
    REQUIRE( C.stack_size() == 0 );
    char const* lua_script = R"(
      list = {}
      for i = 1, 5, 1 do
       list[i] = my_module.hello_to_number( i )
      end
    )";
    REQUIRE( C.loadstring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.getglobal( "list" ) == type::nil );
    REQUIRE( C.stack_size() == 2 );
    C.pop();
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.pcall( /*nargs=*/0, /*nresults=*/0 ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "list" ) == type::table );

    REQUIRE( C.len_pop( -1 ) == 5 );

    REQUIRE( C.geti( -1, 1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "hello world: 1" );
    C.pop();
    REQUIRE( C.geti( -1, 2 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "hello world: 2" );
    C.pop();
    REQUIRE( C.geti( -1, 3 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "hello world: 3" );
    C.pop();
    REQUIRE( C.geti( -1, 4 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "hello world: 4" );
    C.pop();
    REQUIRE( C.geti( -1, 5 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "hello world: 5" );
    C.pop();
    REQUIRE( C.geti( -1, 6 ) == type::nil );
    C.pop();

    C.pop();
    REQUIRE( C.stack_size() == 0 );
  }
}

LUA_TEST_CASE( "[lua-c-api] loadstring" ) {
  REQUIRE( C.getglobal( "xyz" ) == type::nil );
  REQUIRE( C.stack_size() == 1 );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
  char const* script = "xyz = 1 + 2 + 3";
  REQUIRE( C.loadstring( script ) == valid );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.pcall( /*nargs=*/0, /*nresults=*/0 ) == valid );
  REQUIRE( C.stack_size() == 0 );
  REQUIRE( C.getglobal( "xyz" ) == type::number );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.enforce_type_of( -1, type::number ) == valid );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] dostring" ) {
  REQUIRE( C.getglobal( "xyz" ) == type::nil );
  REQUIRE( C.stack_size() == 1 );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
  char const* script = "xyz = 1 + 2 + 3";
  REQUIRE( C.dostring( script ) == valid );
  REQUIRE( C.stack_size() == 0 );
  REQUIRE( C.getglobal( "xyz" ) == type::number );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.enforce_type_of( -1, type::number ) == valid );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] fmt type" ) {
  REQUIRE( fmt::format( "{}", type::nil ) == "nil" );
  REQUIRE( fmt::format( "{}", type::boolean ) == "boolean" );
  REQUIRE( fmt::format( "{}", type::lightuserdata ) ==
           "lightuserdata" );
  REQUIRE( fmt::format( "{}", type::number ) == "number" );
  REQUIRE( fmt::format( "{}", type::string ) == "string" );
  REQUIRE( fmt::format( "{}", type::table ) == "table" );
  REQUIRE( fmt::format( "{}", type::function ) == "function" );
  REQUIRE( fmt::format( "{}", type::userdata ) == "userdata" );
  REQUIRE( fmt::format( "{}", type::thread ) == "thread" );
}

LUA_TEST_CASE( "[lua-c-api] to_str type" ) {
  auto to_str_ = []( type type ) {
    string res;
    to_str( type, res );
    return res;
  };
  REQUIRE( to_str_( type::nil ) == "nil" );
  REQUIRE( to_str_( type::boolean ) == "boolean" );
  REQUIRE( to_str_( type::lightuserdata ) == "lightuserdata" );
  REQUIRE( to_str_( type::number ) == "number" );
  REQUIRE( to_str_( type::string ) == "string" );
  REQUIRE( to_str_( type::table ) == "table" );
  REQUIRE( to_str_( type::function ) == "function" );
  REQUIRE( to_str_( type::userdata ) == "userdata" );
  REQUIRE( to_str_( type::thread ) == "thread" );
}

LUA_TEST_CASE( "[lua-c-api] type_name" ) {
  REQUIRE( C.type_name( type::nil ) == string( "nil" ) );
  REQUIRE( C.type_name( type::boolean ) == string( "boolean" ) );
  // Confirmed that the Lua 5.3 implementation uses "userdata"
  // also for light userdata, not sure wy...
  REQUIRE( C.type_name( type::lightuserdata ) ==
           string( "userdata" ) );
  REQUIRE( C.type_name( type::number ) == string( "number" ) );
  REQUIRE( C.type_name( type::string ) == string( "string" ) );
  REQUIRE( C.type_name( type::table ) == string( "table" ) );
  REQUIRE( C.type_name( type::function ) ==
           string( "function" ) );
  REQUIRE( C.type_name( type::userdata ) ==
           string( "userdata" ) );
  REQUIRE( C.type_name( type::thread ) == string( "thread" ) );
}

LUA_TEST_CASE( "[lua-c-api] push, pop, get, and type_of" ) {
  SECTION( "nil" ) {
    REQUIRE( C.stack_size() == 0 );
    C.push( nil );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::nil );
    REQUIRE( C.get<bool>( -1 ) == false );
    REQUIRE( C.get<boolean>( -1 ) == false );
    REQUIRE( C.get<int>( -1 ) == nothing );
    REQUIRE( C.get<integer>( -1 ) == nothing );
    REQUIRE( C.get<double>( -1 ) == nothing );
    REQUIRE( C.get<floating>( -1 ) == nothing );
    REQUIRE( C.get<string>( -1 ) == nothing );
    C.pop();
  }
  SECTION( "bool" ) {
    REQUIRE( C.stack_size() == 0 );
    C.push( false );
    REQUIRE( C.stack_size() == 1 );
    C.push( true );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::boolean );
    REQUIRE( C.type_of( -2 ) == type::boolean );
    REQUIRE( C.get<bool>( -1 ) == true );
    REQUIRE( C.get<bool>( -2 ) == false );
    REQUIRE( C.get<boolean>( -1 ) == true );
    REQUIRE( C.get<boolean>( -2 ) == false );
    REQUIRE( C.get<int>( -1 ) == nothing );
    REQUIRE( C.get<integer>( -1 ) == nothing );
    REQUIRE( C.get<double>( -1 ) == nothing );
    REQUIRE( C.get<floating>( -1 ) == nothing );
    REQUIRE( C.get<string>( -1 ) == nothing );
    C.pop();
    C.pop();
  }
  SECTION( "integer" ) {
    REQUIRE( C.stack_size() == 0 );
    C.push( int( 0 ) );
    REQUIRE( C.stack_size() == 1 );
    C.push( 9LL );
    REQUIRE( C.stack_size() == 2 );
    C.push( 7L );
    REQUIRE( C.stack_size() == 3 );
    C.push( 5 );
    REQUIRE( C.stack_size() == 4 );
    REQUIRE( C.type_of( -1 ) == type::number );
    REQUIRE( C.type_of( -2 ) == type::number );
    REQUIRE( C.type_of( -3 ) == type::number );
    REQUIRE( C.type_of( -4 ) == type::number );
    REQUIRE( C.get<bool>( -1 ) == true );
    REQUIRE( C.get<bool>( -2 ) == true );
    REQUIRE( C.get<bool>( -3 ) == true );
    REQUIRE( C.get<bool>( -4 ) == true );
    REQUIRE( C.get<boolean>( -1 ) == true );
    REQUIRE( C.get<boolean>( -2 ) == true );
    REQUIRE( C.get<boolean>( -3 ) == true );
    REQUIRE( C.get<boolean>( -4 ) == true );
    REQUIRE( C.get<int>( -1 ) == 5 );
    REQUIRE( C.get<integer>( -1 ) == 5 );
    REQUIRE( C.get<int>( -2 ) == 7 );
    REQUIRE( C.get<integer>( -2 ) == 7 );
    REQUIRE( C.get<int>( -3 ) == 9 );
    REQUIRE( C.get<integer>( -3 ) == 9 );
    REQUIRE( C.get<int>( -4 ) == 0 );
    REQUIRE( C.get<integer>( -4 ) == 0 );
    REQUIRE( C.get<double>( -1 ) == 5.0 );
    REQUIRE( C.get<double>( -2 ) == 7.0 );
    REQUIRE( C.get<floating>( -1 ) == 5.0 );
    REQUIRE( C.get<floating>( -2 ) == 7.0 );
    REQUIRE( C.get<string>( -1 ) == "5" );
    REQUIRE( C.get<string>( -2 ) == "7" );
    REQUIRE( C.get<string>( -3 ) == "9" );
    REQUIRE( C.get<string>( -4 ) == "0" );
    C.pop();
    C.pop();
    C.pop();
    C.pop();
  }
  SECTION( "floating" ) {
    REQUIRE( C.stack_size() == 0 );
    C.push( 7.1 );
    REQUIRE( C.stack_size() == 1 );
    C.push( 5.0f );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::number );
    REQUIRE( C.type_of( -2 ) == type::number );
    REQUIRE( C.get<bool>( -1 ) == true );
    REQUIRE( C.get<bool>( -2 ) == true );
    REQUIRE( C.get<boolean>( -1 ) == true );
    REQUIRE( C.get<boolean>( -2 ) == true );
    REQUIRE( C.get<int>( -1 ) == 5 );
    REQUIRE( C.get<integer>( -1 ) == 5 );
    // No rounding.
    REQUIRE( C.get<int>( -2 ) == nothing );
    REQUIRE( C.get<integer>( -2 ) == nothing );
    REQUIRE( C.get<double>( -1 ) == 5.0 );
    REQUIRE( C.get<double>( -2 ) == 7.1 );
    REQUIRE( C.get<floating>( -1 ) == 5.0 );
    REQUIRE( C.get<floating>( -2 ) == 7.1 );
    REQUIRE( C.get<string>( -1 ) == "5.0" );
    REQUIRE( C.get<string>( -2 ) == "7.1" );
    C.pop();
    C.pop();
  }
  SECTION( "string" ) {
    REQUIRE( C.stack_size() == 0 );
    C.push( string( "5" ) );
    REQUIRE( C.stack_size() == 1 );
    C.push( "hello" );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.type_of( -2 ) == type::string );
    REQUIRE( C.get<bool>( -1 ) == true );
    REQUIRE( C.get<bool>( -2 ) == true );
    REQUIRE( C.get<boolean>( -1 ) == true );
    REQUIRE( C.get<boolean>( -2 ) == true );
    REQUIRE( C.get<int>( -1 ) == nothing );
    REQUIRE( C.get<integer>( -1 ) == nothing );
    REQUIRE( C.get<int>( -2 ) == 5 );
    REQUIRE( C.get<integer>( -2 ) == 5 );
    REQUIRE( C.get<double>( -1 ) == nothing );
    REQUIRE( C.get<double>( -2 ) == 5.0 );
    REQUIRE( C.get<floating>( -1 ) == nothing );
    REQUIRE( C.get<floating>( -2 ) == 5.0 );
    REQUIRE( C.get<string>( -1 ) == "hello" );
    REQUIRE( C.get<string>( -2 ) == "5" );
    C.pop();
    C.pop();
  }
}

LUA_TEST_CASE( "[lua-c-api] call" ) {
  C.openlibs();

  SECTION( "no args, no results" ) {
    char const* lua_script = R"(
      function foo() end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    C.call( /*nargs=*/0, /*nresults=*/0 );
    REQUIRE( C.stack_size() == 0 );
  }

  SECTION( "no args, one result" ) {
    char const* lua_script = R"(
      function foo()
        return 42
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    C.call( /*nargs=*/0, /*nresults=*/1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::number );
    REQUIRE( C.get<int>( -1 ) == 42 );
    REQUIRE( C.get<integer>( -1 ) == 42 );
    C.pop();
  }

  SECTION( "no args, two results" ) {
    char const* lua_script = R"(
      function foo()
        return 42, "hello"
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    C.call( /*nargs=*/0, /*nresults=*/2 );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.type_of( -2 ) == type::number );
    REQUIRE( C.get<string>( -1 ) == "hello" );
    REQUIRE( C.get<int>( -2 ) == 42 );
    REQUIRE( C.get<integer>( -2 ) == 42 );
    C.pop();
    C.pop();
  }

  SECTION( "no args, LUA_MULTRET" ) {
    char const* lua_script = R"(
      function foo()
        return 42, "hello", "world"
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    C.call( /*nargs=*/0, /*nresults=*/LUA_MULTRET );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.type_of( -2 ) == type::string );
    REQUIRE( C.type_of( -3 ) == type::number );
    REQUIRE( C.get<string>( -1 ) == "world" );
    REQUIRE( C.get<string>( -2 ) == "hello" );
    REQUIRE( C.get<int>( -3 ) == 42 );
    REQUIRE( C.get<integer>( -3 ) == 42 );
    C.pop();
    C.pop();
    C.pop();
  }

  SECTION( "one arg, no results" ) {
    char const* lua_script = R"(
      function foo( n )
        assert( n == 5 )
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    C.push( 5 );
    REQUIRE( C.stack_size() == 2 );
    C.call( /*nargs=*/1, /*nresults=*/0 );
    REQUIRE( C.stack_size() == 0 );
  }

  SECTION( "two args, no results" ) {
    char const* lua_script = R"(
      function foo( n, s )
        assert( n == 5 )
        assert( s == "hello" )
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    C.push( 5 );
    C.push( "hello" );
    REQUIRE( C.stack_size() == 3 );
    C.call( /*nargs=*/2, /*nresults=*/0 );
    REQUIRE( C.stack_size() == 0 );
  }

  SECTION( "two args, two results" ) {
    char const* lua_script = R"(
      function foo( n, s )
        assert( n == 42 )
        assert( s == "hello" )
        return n+1, s .. " world"
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    C.push( 42 );
    C.push( "hello" );
    REQUIRE( C.stack_size() == 3 );
    C.call( /*nargs=*/2, /*nresults=*/2 );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.type_of( -2 ) == type::number );
    REQUIRE( C.get<string>( -1 ) == "hello world" );
    REQUIRE( C.get<double>( -2 ) == 43 );
    REQUIRE( C.get<floating>( -2 ) == 43 );
    C.pop();
    C.pop();
  }
}

LUA_TEST_CASE( "[lua-c-api] pcall" ) {
  C.openlibs();

  SECTION( "no args, no results" ) {
    char const* lua_script = R"(
      function foo() end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.pcall( /*nargs=*/0, /*nresults=*/0 ) == valid );
    REQUIRE( C.stack_size() == 0 );
  }

  SECTION( "no args, one result" ) {
    char const* lua_script = R"(
      function foo()
        return 42
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.pcall( /*nargs=*/0, /*nresults=*/1 ) == valid );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::number );
    REQUIRE( C.get<int>( -1 ) == 42 );
    REQUIRE( C.get<integer>( -1 ) == 42 );
    C.pop();
  }

  SECTION( "no args, two results" ) {
    char const* lua_script = R"(
      function foo()
        return 42, "hello"
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.pcall( /*nargs=*/0, /*nresults=*/2 ) == valid );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.type_of( -2 ) == type::number );
    REQUIRE( C.get<string>( -1 ) == "hello" );
    REQUIRE( C.get<int>( -2 ) == 42 );
    REQUIRE( C.get<integer>( -2 ) == 42 );
    C.pop();
    C.pop();
  }

  SECTION( "no args, LUA_MULTRET" ) {
    char const* lua_script = R"(
      function foo()
        return 42, "hello", "world"
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.pcall( /*nargs=*/0, /*nresults=*/LUA_MULTRET ) ==
             valid );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.type_of( -2 ) == type::string );
    REQUIRE( C.type_of( -3 ) == type::number );
    REQUIRE( C.get<string>( -1 ) == "world" );
    REQUIRE( C.get<string>( -2 ) == "hello" );
    REQUIRE( C.get<int>( -3 ) == 42 );
    REQUIRE( C.get<integer>( -3 ) == 42 );
    C.pop();
    C.pop();
    C.pop();
  }

  SECTION( "one arg, no results" ) {
    char const* lua_script = R"(
      function foo( n )
        assert( n == 5 )
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    C.push( 5 );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.pcall( /*nargs=*/1, /*nresults=*/0 ) == valid );
    REQUIRE( C.stack_size() == 0 );
  }

  SECTION( "one arg, error" ) {
    char const* lua_script = R"(
      function foo( n )
        assert( n == 5 )
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    C.push( 6 );
    REQUIRE( C.stack_size() == 2 );
    // clang-format off
    char const* err =
      "[string \"...\"]:3: assertion failed!"  "\n"
      "stack traceback:"                       "\n"
      "\t[C]: in function 'assert'"            "\n"
      "\t[string \"...\"]:3: in function 'foo'";
    // clang-format on
    REQUIRE( C.pcall( /*nargs=*/1, /*nresults=*/0 ) ==
             lua_invalid( err ) );
    REQUIRE( C.stack_size() == 0 );
  }

  SECTION( "two args, no results" ) {
    char const* lua_script = R"(
      function foo( n, s )
        assert( n == 5 )
        assert( s == "hello" )
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    C.push( 5 );
    C.push( "hello" );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.pcall( /*nargs=*/2, /*nresults=*/0 ) == valid );
    REQUIRE( C.stack_size() == 0 );
  }

  SECTION( "two args, two results" ) {
    char const* lua_script = R"(
      function foo( n, s )
        assert( n == 42 )
        assert( s == "hello" )
        return n+1, s .. " world"
      end
    )";
    REQUIRE( C.dostring( lua_script ) == valid );
    REQUIRE( C.stack_size() == 0 );
    REQUIRE( C.getglobal( "foo" ) == type::function );
    REQUIRE( C.stack_size() == 1 );
    C.push( 42 );
    C.push( "hello" );
    REQUIRE( C.stack_size() == 3 );
    REQUIRE( C.pcall( /*nargs=*/2, /*nresults=*/2 ) == valid );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.type_of( -2 ) == type::number );
    REQUIRE( C.get<string>( -1 ) == "hello world" );
    REQUIRE( C.get<double>( -2 ) == 43 );
    REQUIRE( C.get<floating>( -2 ) == 43 );
    C.pop();
    C.pop();
  }
}

LUA_TEST_CASE( "[lua-c-api] type to string" ) {
  REQUIRE( fmt::format( "{}", type::nil ) == "nil" );
  REQUIRE( fmt::format( "{}", type::boolean ) == "boolean" );
  REQUIRE( fmt::format( "{}", type::lightuserdata ) ==
           "lightuserdata" );
  REQUIRE( fmt::format( "{}", type::number ) == "number" );
  REQUIRE( fmt::format( "{}", type::string ) == "string" );
  REQUIRE( fmt::format( "{}", type::table ) == "table" );
  REQUIRE( fmt::format( "{}", type::function ) == "function" );
  REQUIRE( fmt::format( "{}", type::userdata ) == "userdata" );
  REQUIRE( fmt::format( "{}", type::thread ) == "thread" );
}

LUA_TEST_CASE( "[lua-c-api] newtable" ) {
  REQUIRE( C.stack_size() == 0 );
  C.newtable();
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::table );
  C.pop();
}

LUA_TEST_CASE(
    "[lua-c-api] push_global_table, settable, gettable, "
    "getfield, setfield" ) {
  REQUIRE( C.getglobal( "hello" ) == type::nil );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
  C.pushglobaltable();
  REQUIRE( C.stack_size() == 1 );
  C.push( "hello" );
  C.push( 3.5 );
  REQUIRE( C.stack_size() == 3 );
  C.settable( -3 );
  REQUIRE( C.stack_size() == 1 );

  C.push( "hello" );
  REQUIRE( C.gettable( -2 ) == type::number );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.get<double>( -1 ) == 3.5 );
  C.pop();
  REQUIRE( C.stack_size() == 1 );

  // At this point, only the global table is still on the stack.
  C.pop();
  // Not anymore.
  REQUIRE( C.stack_size() == 0 );

  REQUIRE( C.getglobal( "hello" ) == type::number );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::number );
  REQUIRE( C.get<double>( -1 ) == 3.5 );
  REQUIRE( C.get<floating>( -1 ) == 3.5 );
  C.pop();

  C.pushglobaltable();
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.getfield( -1, "hello" ) == type::number );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.get<double>( -1 ) == 3.5 );
  REQUIRE( C.get<floating>( -1 ) == 3.5 );
  C.pop( 1 );

  // At this point, the global table is on the stack.
  REQUIRE( C.stack_size() == 1 );
  C.push( true );
  C.setfield( -2, "hello" );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::table );
  REQUIRE( C.getfield( -1, "hello" ) == type::boolean );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.get<bool>( -1 ) == true );
  REQUIRE( C.get<boolean>( -1 ) == true );
  C.pop( 2 );
}

LUA_TEST_CASE( "[lua-c-api] rawgeti, rawseti" ) {
  C.pushglobaltable();
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.rawgeti( -1, 42 ) == type::nil );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.type_of( -1 ) == type::nil );
  C.pop();
  REQUIRE( C.stack_size() == 1 );

  C.push( "world" );
  C.rawseti( -2, 42 );
  REQUIRE( C.stack_size() == 1 );

  REQUIRE( C.rawgeti( -1, 42 ) == type::string );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.type_of( -1 ) == type::string );
  REQUIRE( C.get<string>( -1 ) == "world" );
  C.pop( 2 );
}

LUA_TEST_CASE(
    "[lua-c-api] ref/ref_registry/registry_get/unref" ) {
  C.push( 5 );
  int r1 = C.ref_registry();
  C.push( "hello" );
  int r2 = C.ref_registry();
  REQUIRE( r1 != r2 );
  REQUIRE( C.stack_size() == 0 );

  REQUIRE( C.registry_get( r1 ) == type::number );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.get<int>( -1 ) == 5 );
  REQUIRE( C.get<integer>( -1 ) == 5 );
  C.pop();
  REQUIRE( C.registry_get( r2 ) == type::string );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.get<string>( -1 ) == "hello" );
  C.pop();
}

LUA_TEST_CASE( "[lua-c-api] len" ) {
  // Table length.
  REQUIRE( C.dostring( "x = {4,5,6}" ) == valid );
  REQUIRE( C.getglobal( "x" ) == type::table );
  REQUIRE( C.len_pop( -1 ) == 3 );
  C.pop();
  REQUIRE( C.stack_size() == 0 );

  // String length.
  REQUIRE( C.dostring( "y = 'hello'" ) == valid );
  REQUIRE( C.getglobal( "y" ) == type::string );
  REQUIRE( C.len_pop( -1 ) == 5 );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] geti" ) {
  REQUIRE(
      C.dostring( "x = {[4]='one',[5]='two',[6]='three'}" ) ==
      valid );
  REQUIRE( C.getglobal( "x" ) == type::table );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.geti( -1, 5 ) == type::string );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.get<string>( -1 ) == "two" );
  C.pop( 2 );
  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] push c function" ) {
  C.openlibs();

  C.push( []( lua_State* L ) -> int {
    int n = luaL_checkinteger( L, 1 );
    lua_pushinteger( L, n + 3 );
    return 1;
  } );
  C.setglobal( "bar" );

  REQUIRE( C.dostring( R"(
    local input  = 7
    local expect = 10
    local output    = bar( input )
    assert( output == expect,
            tostring( output ) .. ' != ' .. tostring( expect ) )
  )" ) == valid );
}

LUA_TEST_CASE(
    "[lua-c-api] push c function with upvalues + getupvalue" ) {
  C.openlibs();

  REQUIRE( C.stack_size() == 0 );
  C.push( 42 );
  C.push(
      []( lua_State* L ) -> int {
        int n = luaL_checkinteger( L, 1 );
        int upvalue =
            luaL_checkinteger( L, lua_upvalueindex( 1 ) );
        lua_pushinteger( L, n + upvalue );
        return 1;
      },
      /*upvalues=*/1 );
  // Consumes the upvalue but leaves the closure on the stack.
  REQUIRE( C.stack_size() == 1 );
  C.setglobal( "bar" );

  REQUIRE( C.dostring( R"(
    local input  = 7
    local expect = 49 -- 7+42
    local output    = bar( input )
    assert( output == expect,
            tostring( output ) .. ' != ' .. tostring( expect ) )
  )" ) == valid );

  // Test that the function has an up value and that the upvalue
  // has the right type.
  C.getglobal( "bar" );
  REQUIRE( C.type_of( -1 ) == type::function );
  REQUIRE_FALSE( C.getupvalue( -1, 2 ) );
  REQUIRE( C.getupvalue( -1, 1 ) == true );
  REQUIRE( C.type_of( -1 ) == type::number );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.get<int>( -1 ) == 42 );
  REQUIRE( C.get<integer>( -1 ) == 42 );
  C.pop( 2 );
  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] insert" ) {
  REQUIRE( C.stack_size() == 0 );
  C.push( 5 );
  C.push( "hello" );
  C.push( true );
  C.push( 7 );
  REQUIRE( C.type_of( -1 ) == type::number );
  REQUIRE( C.type_of( -2 ) == type::boolean );
  REQUIRE( C.type_of( -3 ) == type::string );
  REQUIRE( C.type_of( -4 ) == type::number );

  C.insert( -3 );
  REQUIRE( C.type_of( -1 ) == type::boolean );
  REQUIRE( C.type_of( -2 ) == type::string );
  REQUIRE( C.type_of( -3 ) == type::number );
  REQUIRE( C.type_of( -4 ) == type::number );
  REQUIRE( C.get<int>( -3 ) == 7 );
  REQUIRE( C.get<integer>( -3 ) == 7 );
  REQUIRE( C.get<int>( -4 ) == 5 );
  REQUIRE( C.get<integer>( -4 ) == 5 );

  C.pop( 4 );
}

LUA_TEST_CASE( "[lua-c-api] swap_top" ) {
  REQUIRE( C.stack_size() == 0 );
  C.push( 5 );
  C.push( "hello" );
  C.push( true );
  C.push( 7 );
  REQUIRE( C.type_of( -1 ) == type::number );
  REQUIRE( C.type_of( -2 ) == type::boolean );
  REQUIRE( C.type_of( -3 ) == type::string );
  REQUIRE( C.type_of( -4 ) == type::number );

  C.swap_top();
  REQUIRE( C.type_of( -1 ) == type::boolean );
  REQUIRE( C.type_of( -2 ) == type::number );
  REQUIRE( C.type_of( -3 ) == type::string );
  REQUIRE( C.type_of( -4 ) == type::number );
  REQUIRE( C.get<int>( -2 ) == 7 );
  REQUIRE( C.get<integer>( -2 ) == 7 );

  C.pop( 4 );
}

LUA_TEST_CASE( "[lua-c-api] setmetatable/getmetatable" ) {
  C.openlibs();

  C.newtable();
  C.newtable(); // metatable
  REQUIRE( C.stack_size() == 2 );
  C.pushglobaltable();
  REQUIRE( C.stack_size() == 3 );
  C.setfield( -2, "__index" );
  REQUIRE( C.stack_size() == 2 );
  C.setmetatable( -2 );
  REQUIRE( C.stack_size() == 1 );
  C.setglobal( "x" );
  REQUIRE( C.stack_size() == 0 );
  REQUIRE( C.dostring( R"(
    x.assert( x.print ~= nil )
  )" ) == valid );
  REQUIRE( C.stack_size() == 0 );

  C.getglobal( "x" );
  REQUIRE( C.stack_size() == 1 );
  C.getmetatable( -1 );
  REQUIRE( C.stack_size() == 2 );
  C.push( nil );
  REQUIRE( C.stack_size() == 3 );
  C.setfield( -2, "__index" );
  REQUIRE( C.stack_size() == 2 );
  C.pop( 2 );
  REQUIRE( C.stack_size() == 0 );

  char const* err =
      "[string \"...\"]:2: field 'assert' is not callable (a "
      "nil value)\n"
      "stack traceback:\n"
      "\t[string \"...\"]:2: in main chunk";

  REQUIRE( C.dostring( R"(
    x.assert( x.print ~= nil )
  )" ) == lua_invalid( err ) );
}

LUA_TEST_CASE( "[lua-c-api] newuserdata" ) {
  void* p = C.newuserdata( 1 );
  REQUIRE( C.type_of( -1 ) == type::userdata );
  REQUIRE( C.get<void*>( -1 ) == p );
  REQUIRE( C.get<lightuserdata>( -1 ) == p );
  C.pop();
}

LUA_TEST_CASE(
    "[lua-c-api] udata_{new,get,set}metatable + "
    "{check,test}udata" ) {
  REQUIRE( C.udata_getmetatable( "hello" ) == type::nil );
  REQUIRE( C.stack_size() == 1 );
  C.pop();
  REQUIRE( C.udata_newmetatable( "hello" ) == true );
  REQUIRE( C.stack_size() == 1 );
  C.pop();
  REQUIRE( C.udata_newmetatable( "hello" ) == false );
  REQUIRE( C.stack_size() == 1 );
  C.pop();
  REQUIRE( C.udata_getmetatable( "hello" ) == type::table );
  REQUIRE( C.stack_size() == 1 );
  C.getfield( -1, "__name" );
  REQUIRE( C.type_of( -1 ) == type::string );
  REQUIRE( C.get<string>( -1 ) == "hello" );
  C.pop( 2 );
  REQUIRE( C.stack_size() == 0 );

  REQUIRE( C.newuserdata( 1 ) != nullptr );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.testudata( -1, "hello" ) == nullptr );
  REQUIRE( C.stack_size() == 1 );
  C.udata_setmetatable( "hello" );
  REQUIRE( C.stack_size() == 1 );

  REQUIRE( C.testudata( -1, "hello" ) != nullptr );
  C.pop();
}

LUA_TEST_CASE( "[lua-c-api] error" ) {
  SECTION( "error push" ) {
    C.push( []( lua_State* L ) -> int {
      c_api C( L );
      C.push( "this is an error." );
      C.error();
      return 0;
    } );
    // clang-format off
    char const* err =
      "this is an error.\n"
      "stack traceback:\n"
      "\t[C]: in ?";
    // clang-format on
    REQUIRE( C.pcall( 0, 0 ) == lua_invalid( err ) );
  }

  SECTION( "error arg" ) {
    C.push( []( lua_State* L ) -> int {
      c_api C( L );
      C.error( "this is an error." );
      return 0;
    } );
    // clang-format off
    char const* err =
      "this is an error.\n"
      "stack traceback:\n"
      "\t[C]: in ?";
    // clang-format on
    REQUIRE( C.pcall( 0, 0 ) == lua_invalid( err ) );
  }
}

LUA_TEST_CASE( "[lua-c-api] constants" ) {
  REQUIRE( c_api::noref() == LUA_NOREF );
  REQUIRE( c_api::multret() == LUA_MULTRET );
}

LUA_TEST_CASE( "[lua-c-api] pushvalue" ) {
  C.push( 5 );
  REQUIRE( C.stack_size() == 1 );
  C.pushvalue( -1 );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.get<int>( -1 ) == 5 );
  REQUIRE( C.get<integer>( -1 ) == 5 );
  C.push( 7 );
  REQUIRE( C.stack_size() == 3 );
  REQUIRE( C.get<int>( -1 ) == 7 );
  REQUIRE( C.get<integer>( -1 ) == 7 );
  C.pushvalue( -2 );
  REQUIRE( C.stack_size() == 4 );
  REQUIRE( C.get<int>( -1 ) == 5 );
  REQUIRE( C.get<integer>( -1 ) == 5 );
  C.pop( 4 );
  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] compare" ) {
  SECTION( "eq" ) {
    C.push( 5 );
    C.push( 5 );
    REQUIRE( C.compare_eq( -2, -1 ) );
    REQUIRE( C.compare_eq( -1, -2 ) );
    C.push( 6 );
    REQUIRE_FALSE( C.compare_eq( -2, -1 ) );
    REQUIRE_FALSE( C.compare_eq( -1, -2 ) );
    C.pop( 3 );
    REQUIRE( C.stack_size() == 0 );
  }

  SECTION( "lt" ) {
    C.push( 5 );
    C.push( 5 );
    REQUIRE_FALSE( C.compare_lt( -2, -1 ) );
    REQUIRE_FALSE( C.compare_lt( -1, -2 ) );
    C.push( 6 );
    REQUIRE( C.compare_lt( -2, -1 ) );
    REQUIRE_FALSE( C.compare_lt( -1, -2 ) );
    C.pop( 3 );
    REQUIRE( C.stack_size() == 0 );
  }

  SECTION( "le" ) {
    C.push( 5 );
    C.push( 5 );
    REQUIRE( C.compare_le( -2, -1 ) );
    REQUIRE( C.compare_le( -1, -2 ) );
    C.push( 6 );
    REQUIRE( C.compare_le( -2, -1 ) );
    REQUIRE_FALSE( C.compare_le( -1, -2 ) );
    C.pop( 3 );
    REQUIRE( C.stack_size() == 0 );
  }
}

LUA_TEST_CASE( "[lua-c-api] concat" ) {
  SECTION( "empty" ) {
    C.concat( 0 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "" );
    C.pop();
  }

  SECTION( "single empty string" ) {
    C.push( "" );
    C.concat( 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "" );
    C.pop();
  }

  SECTION( "single non-empty string" ) {
    C.push( "hello" );
    C.concat( 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "hello" );
    C.pop();
  }

  SECTION( "two strings" ) {
    C.push( "hello" );
    C.push( "world" );
    C.concat( 2 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "helloworld" );
    C.pop();
  }

  SECTION( "single nil" ) {
    C.push( nil );
    C.concat( 1 );
    REQUIRE( C.stack_size() == 1 );
    // no dice.
    REQUIRE( C.type_of( -1 ) == type::nil );
    C.pop();
  }

  SECTION( "single bool" ) {
    C.push( true );
    C.concat( 1 );
    REQUIRE( C.stack_size() == 1 );
    // no dice.
    REQUIRE( C.type_of( -1 ) == type::boolean );
    REQUIRE( C.get<bool>( -1 ) == true );
    REQUIRE( C.get<boolean>( -1 ) == true );
    C.pop();
  }

  SECTION( "single number" ) {
    C.push( 5 );
    C.concat( 1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::number );
    C.pop();
  }

  SECTION( "two numbers" ) {
    C.push( 5 );
    C.push( 4.5 );
    C.concat( 2 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "54.5" );
    C.pop();
  }

  SECTION( "numbers and strings" ) {
    C.push( 5 );
    C.push( "hello" );
    C.push( 4.5 );
    C.push( "4.5" );
    C.concat( 4 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "5hello4.54.5" );
    C.pop();
  }

  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] tostring" ) {
  size_t len = 10000;

  SECTION( "nil" ) {
    C.push( nil );
    REQUIRE( string( C.tostring( -1, &len ) ) == "nil" );
    REQUIRE( len == 3 );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "nil" );
    C.pop( 2 );
  }

  SECTION( "empty string" ) {
    C.push( "" );
    REQUIRE( string( C.tostring( -1, &len ) ) == "" );
    REQUIRE( len == 0 );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "" );
    C.pop( 2 );
  }

  SECTION( "non-empty string" ) {
    C.push( "hello" );
    REQUIRE( string( C.tostring( -1, &len ) ) == "hello" );
    REQUIRE( len == 5 );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "hello" );
    C.pop( 2 );
  }

  SECTION( "bool" ) {
    C.push( true );
    REQUIRE( string( C.tostring( -1, &len ) ) == "true" );
    REQUIRE( len == 4 );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "true" );
    C.pop( 2 );
  }

  SECTION( "integer" ) {
    C.push( 5 );
    REQUIRE( string( C.tostring( -1, &len ) ) == "5" );
    REQUIRE( len == 1 );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "5" );
    C.pop( 2 );
  }

  SECTION( "float" ) {
    C.push( 5.5 );
    REQUIRE( string( C.tostring( -1, &len ) ) == "5.5" );
    REQUIRE( len == 3 );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE( C.get<string>( -1 ) == "5.5" );
    C.pop( 2 );
  }

  SECTION( "table" ) {
    C.newtable();
    REQUIRE_THAT( C.tostring( -1, &len ),
                  Matches( "table: 0x[0-9a-z]+" ) );
    REQUIRE( len > 9 );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE_THAT( *C.get<string>( -1 ),
                  Matches( "table: 0x[0-9a-z]+" ) );
    C.pop( 2 );
  }

  SECTION( "function" ) {
    C.push( []( lua_State* ) -> int { return 0; } );
    REQUIRE_THAT( C.tostring( -1, &len ),
                  Matches( "function: 0x[0-9a-z]+" ) );
    REQUIRE( len > 12 );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE_THAT( *C.get<string>( -1 ),
                  Matches( "function: 0x[0-9a-z]+" ) );
    C.pop( 2 );
  }

  SECTION( "lightuserdata" ) {
    int x = 0;
    C.push( (void*)&x );
    REQUIRE_THAT( C.tostring( -1, &len ),
                  Matches( "userdata: 0x[0-9a-z]+" ) );
    REQUIRE( len > 12 );
    REQUIRE( C.stack_size() == 2 );
    REQUIRE( C.type_of( -1 ) == type::string );
    REQUIRE_THAT( *C.get<string>( -1 ),
                  Matches( "userdata: 0x[0-9a-z]+" ) );
    C.pop( 2 );
  }

  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] isinteger" ) {
  // bool
  C.push( true );
  REQUIRE_FALSE( C.isinteger( -1 ) );
  // integer
  C.push( 5 );
  REQUIRE( C.isinteger( -1 ) );
  // integer as double
  C.push( 5.0 );
  REQUIRE_FALSE( C.isinteger( -1 ) );
  // double
  C.push( 5.5 );
  REQUIRE_FALSE( C.isinteger( -1 ) );
  // string
  C.push( "5" );
  REQUIRE_FALSE( C.isinteger( -1 ) );
  // table
  C.newtable();
  REQUIRE_FALSE( C.isinteger( -1 ) );

  REQUIRE( C.stack_size() == 6 );
  C.pop( 6 );
}

// This demonstrates that different threads within a state have
// separate stacks.
LUA_TEST_CASE( "[lua-c-api] separate thread stacks" ) {
  cthread thread1 = C.this_cthread();
  cthread thread2 = C.newthread();
  cthread thread3 = C.newthread();

  c_api view1( thread1 );
  c_api view2( thread2 );
  c_api view3( thread3 );

  REQUIRE( view1.stack_size() == 2 );
  REQUIRE( view2.stack_size() == 0 );
  REQUIRE( view3.stack_size() == 0 );

  view1.push( 5 );
  REQUIRE( view1.stack_size() == 3 );
  REQUIRE( view2.stack_size() == 0 );
  REQUIRE( view3.stack_size() == 0 );

  view2.push( 5 );
  REQUIRE( view1.stack_size() == 3 );
  REQUIRE( view2.stack_size() == 1 );
  REQUIRE( view3.stack_size() == 0 );

  view3.push( 5 );
  REQUIRE( view1.stack_size() == 3 );
  REQUIRE( view2.stack_size() == 1 );
  REQUIRE( view3.stack_size() == 1 );

  view3.pop();
  REQUIRE( view1.stack_size() == 3 );
  REQUIRE( view2.stack_size() == 1 );
  REQUIRE( view3.stack_size() == 0 );

  view2.pop();
  REQUIRE( view1.stack_size() == 3 );
  REQUIRE( view2.stack_size() == 0 );
  REQUIRE( view3.stack_size() == 0 );

  view1.pop( 3 );
  REQUIRE( view1.stack_size() == 0 );
  REQUIRE( view2.stack_size() == 0 );
  REQUIRE( view3.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] pushthread" ) {
  REQUIRE( C.pushthread() );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.type_of( -1 ) == type::thread );

  c_api C2 = C.newthread();
  // The new thread gets pushed into the stack of C.
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C2.stack_size() == 0 );
  REQUIRE_FALSE( C2.pushthread() );
  REQUIRE( C2.stack_size() == 1 );
  REQUIRE( C2.type_of( -1 ) == type::thread );

  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C2.stack_size() == 1 );

  C.pop( 2 );
  C2.pop();

  REQUIRE( C.stack_size() == 0 );
  REQUIRE( C2.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] pop_tostring" ) {
  C.push( 5 );
  REQUIRE( C.pop_tostring() == "5" );
  REQUIRE( C.stack_size() == 0 );

  C.push( 5.5 );
  REQUIRE( C.pop_tostring() == "5.5" );
  REQUIRE( C.stack_size() == 0 );

  C.newtable();
  REQUIRE_THAT( C.pop_tostring(),
                Matches( "table: 0x[0-9a-z]+$" ) );
  REQUIRE( C.stack_size() == 0 );

  C.push( "hello" );
  REQUIRE( C.pop_tostring() == "hello" );
  REQUIRE( C.stack_size() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] rawlen" ) {
  C.push( "hello" );
  REQUIRE( C.rawlen( -1 ) == 5 );
  C.pop();

  C.newtable();
  REQUIRE( C.rawlen( -1 ) == 0 );
  C.push( "hello" );
  C.push( 5 );
  C.settable( -3 );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.rawlen( -1 ) == 0 );
  C.push( 1 );
  C.push( "hello" );
  C.settable( -3 );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C.rawlen( -1 ) == 1 );
  C.pop();

  C.newuserdata( 1000 );
  REQUIRE( C.rawlen( -1 ) == 1000 );
  C.pop();
}

LUA_TEST_CASE( "[lua-c-api] checkinteger" ) {
  C.push( []( lua_State* L ) -> int {
    c_api C( L );
    int   n = C.checkinteger( 1 );
    C.push( fmt::format( "hello {}", n ) );
    return 1;
  } );
  REQUIRE( C.stack_size() == 1 );

  SECTION( "good" ) {
    C.push( 3 );
    REQUIRE( C.stack_size() == 2 );
    C.call( /*nargs=*/1, /*nresults=*/1 );
    REQUIRE( C.stack_size() == 1 );
    REQUIRE( C.get<string>( -1 ) == "hello 3" );
    C.pop();
  }

  SECTION( "bad" ) {
    C.push( "world" );
    REQUIRE( C.stack_size() == 2 );
    char const* err =
        "bad argument #1 to '?' (number expected, got string)\n"
        "stack traceback:\n"
        "\t[C]: in ?";

    REQUIRE( C.pcall( /*nargs=*/1, /*nresults=*/1 ) ==
             lua_invalid( err ) );
  }
}

LUA_TEST_CASE( "[lua-c-api] settop" ) {
  C.push( 1 );
  C.push( 1 );
  C.push( 1 );
  REQUIRE( C.gettop() == 3 );
  C.settop( 5 );
  REQUIRE( C.gettop() == 5 );
  REQUIRE( C.type_of( -1 ) == type::nil );
  REQUIRE( C.type_of( -2 ) == type::nil );
  REQUIRE( C.type_of( -3 ) == type::number );
  C.settop( 2 );
  REQUIRE( C.gettop() == 2 );
  REQUIRE( C.type_of( -1 ) == type::number );
  REQUIRE( C.type_of( -2 ) == type::number );
  C.settop( 0 );
  REQUIRE( C.gettop() == 0 );
}

LUA_TEST_CASE( "[lua-c-api] tothread" ) {
  CHECK( C.pushthread() );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( L == C.tothread( -1 ) );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
  cthread cth2 = C.newthread();
  cthread cth3 = C.tothread( -1 );
  REQUIRE( cth3 == cth2 );
  c_api C2( cth2 );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C2.stack_size() == 0 );
  REQUIRE( !C2.pushthread() );
  REQUIRE( C.stack_size() == 1 );
  REQUIRE( C2.stack_size() == 1 );
  C.pop();
  C2.pop();
  REQUIRE( C.stack_size() == 0 );
  REQUIRE( C2.stack_size() == 0 );
}

} // namespace
} // namespace lua
