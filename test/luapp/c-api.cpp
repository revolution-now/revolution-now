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

// Lua
#include "lauxlib.h"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::luapp::e_lua_type );

namespace luapp {
namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::base::valid;

string lua_testing_file( string const& filename ) {
  return rn::testing::data_dir() / "lua" / filename;
}

TEST_CASE( "[lua-c-api] create and destroy" ) { c_api st; }

TEST_CASE( "[lua-c-api] openlibs" ) {
  c_api st;
  REQUIRE( st.getglobal( "tostring" ) == e_lua_type::nil );
  REQUIRE( st.stack_size() == 1 );
  st.pop();
  REQUIRE( st.stack_size() == 0 );
  st.openlibs();
  REQUIRE( st.getglobal( "tostring" ) == e_lua_type::function );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.enforce_type_of( -1, e_lua_type::function ) ==
           valid );
  st.pop();
  REQUIRE( st.stack_size() == 0 );
}

TEST_CASE( "[lua-c-api] rotate" ) {
  c_api st;
  st.push( true );
  st.push( "hello" );
  st.push( false );
  st.push( 3.5 );
  REQUIRE( st.stack_size() == 4 );
  REQUIRE( st.enforce_type_of( -1, e_lua_type::number ) ==
           valid );
  REQUIRE( st.enforce_type_of( -2, e_lua_type::boolean ) ==
           valid );
  REQUIRE( st.enforce_type_of( -3, e_lua_type::string ) ==
           valid );
  REQUIRE( st.enforce_type_of( -4, e_lua_type::boolean ) ==
           valid );

  st.rotate( -3, 2 );

  REQUIRE( st.stack_size() == 4 );
  REQUIRE( st.enforce_type_of( -1, e_lua_type::string ) ==
           valid );
  REQUIRE( st.enforce_type_of( -2, e_lua_type::number ) ==
           valid );
  REQUIRE( st.enforce_type_of( -3, e_lua_type::boolean ) ==
           valid );
  REQUIRE( st.enforce_type_of( -4, e_lua_type::boolean ) ==
           valid );

  st.pop( 4 );
}

TEST_CASE( "[lua-c-api] {get,set}global" ) {
  c_api st;
  REQUIRE( st.getglobal( "xyz" ) == e_lua_type::nil );
  REQUIRE( st.stack_size() == 1 );
  st.pop();
  REQUIRE( st.stack_size() == 0 );
  st.push( 1 );
  st.setglobal( "xyz" );
  REQUIRE( st.stack_size() == 0 );
  REQUIRE( st.getglobal( "xyz" ) == e_lua_type::number );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.get<int>( -1 ) == 1 );
  st.pop();
}

TEST_CASE( "[lua-c-api] getglobal with __index/error" ) {
  c_api st;
  st.openlibs();
  REQUIRE( st.dostring( R"(
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
  REQUIRE( st.getglobal_safe( "xyz" ) ==
           lua_unexpected<e_lua_type>( err ) );
  REQUIRE( st.stack_size() == 0 );
  REQUIRE( st.dostring( "xyz = 1" ) == valid );
  REQUIRE( st.getglobal( "xyz" ) == e_lua_type::number );
  REQUIRE( st.stack_size() == 1 );
  st.pop();
}

TEST_CASE( "[lua-c-api] setglobal with __index/error" ) {
  c_api st;
  st.openlibs();
  REQUIRE( st.dostring( R"(
    setmetatable( _G, {
      __newindex = function( k )
        error( 'this is an error.' )
      end
    } )
  )" ) == valid );
  REQUIRE( st.getglobal( "xyz" ) == e_lua_type::nil );
  REQUIRE( st.stack_size() == 1 );
  st.pop();
  REQUIRE( st.stack_size() == 0 );
  st.push( 1 );
  // clang-format off
  char const* err =
    "[string \"...\"]:4: this is an error."                  "\n"
    "stack traceback:"                                       "\n"
    "\t[C]: in function 'error'"                             "\n"
    "\t[string \"...\"]:4: in function <[string \"...\"]:3>" "\n"
    "\t[C]: in ?";
  // clang-format on
  REQUIRE( st.setglobal_safe( "xyz" ) == lua_invalid( err ) );

  // The pinvoke will get rid of the parameter.
  REQUIRE( st.stack_size() == 0 );
}

TEST_CASE( "[lua-c-api] dofile" ) {
  c_api st;
  st.openlibs();

  SECTION( "non-existent" ) {
    REQUIRE( st.dofile( lua_testing_file( "xxx.lua" ) ) ==
             lua_invalid( "cannot open test/data/lua/xxx.lua: "
                          "No such file or directory" ) );
    REQUIRE( st.stack_size() == 0 );
  }

  SECTION( "exists" ) {
    REQUIRE( st.dofile( lua_testing_file(
                 "c-api-dofile.lua" ) ) == valid );
    REQUIRE( st.stack_size() == 1 );
    REQUIRE( st.enforce_type_of( -1, e_lua_type::table ) ==
             valid );
    st.setglobal( "my_module" );
    REQUIRE( st.stack_size() == 0 );
    char const* lua_script = R"(
      list = {}
      for i = 1, 5, 1 do
       list[i] = my_module.hello_to_number( i )
      end
    )";
    REQUIRE( st.loadstring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 1 );
    REQUIRE( st.getglobal( "list" ) == e_lua_type::nil );
    REQUIRE( st.stack_size() == 2 );
    st.pop();
    REQUIRE( st.stack_size() == 1 );
    REQUIRE( st.pcall( /*nargs=*/0, /*nresults=*/0 ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "list" ) == e_lua_type::table );

    REQUIRE( st.len_pop( -1 ) == 5 );

    REQUIRE( st.geti( -1, 1 ) == e_lua_type::string );
    REQUIRE( st.get<string>( -1 ) == "hello world: 1" );
    st.pop();
    REQUIRE( st.geti( -1, 2 ) == e_lua_type::string );
    REQUIRE( st.get<string>( -1 ) == "hello world: 2" );
    st.pop();
    REQUIRE( st.geti( -1, 3 ) == e_lua_type::string );
    REQUIRE( st.get<string>( -1 ) == "hello world: 3" );
    st.pop();
    REQUIRE( st.geti( -1, 4 ) == e_lua_type::string );
    REQUIRE( st.get<string>( -1 ) == "hello world: 4" );
    st.pop();
    REQUIRE( st.geti( -1, 5 ) == e_lua_type::string );
    REQUIRE( st.get<string>( -1 ) == "hello world: 5" );
    st.pop();
    REQUIRE( st.geti( -1, 6 ) == e_lua_type::nil );
    st.pop();

    st.pop();
    REQUIRE( st.stack_size() == 0 );
  }
}

TEST_CASE( "[lua-c-api] loadstring" ) {
  c_api st;
  REQUIRE( st.getglobal( "xyz" ) == e_lua_type::nil );
  REQUIRE( st.stack_size() == 1 );
  st.pop();
  REQUIRE( st.stack_size() == 0 );
  char const* script = "xyz = 1 + 2 + 3";
  REQUIRE( st.loadstring( script ) == valid );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.pcall( /*nargs=*/0, /*nresults=*/0 ) == valid );
  REQUIRE( st.stack_size() == 0 );
  REQUIRE( st.getglobal( "xyz" ) == e_lua_type::number );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.enforce_type_of( -1, e_lua_type::number ) ==
           valid );
  st.pop();
  REQUIRE( st.stack_size() == 0 );
}

TEST_CASE( "[lua-c-api] dostring" ) {
  c_api st;
  REQUIRE( st.getglobal( "xyz" ) == e_lua_type::nil );
  REQUIRE( st.stack_size() == 1 );
  st.pop();
  REQUIRE( st.stack_size() == 0 );
  char const* script = "xyz = 1 + 2 + 3";
  REQUIRE( st.dostring( script ) == valid );
  REQUIRE( st.stack_size() == 0 );
  REQUIRE( st.getglobal( "xyz" ) == e_lua_type::number );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.enforce_type_of( -1, e_lua_type::number ) ==
           valid );
  st.pop();
  REQUIRE( st.stack_size() == 0 );
}

TEST_CASE( "[lua-c-api] fmt e_lua_type" ) {
  c_api st;
  REQUIRE( fmt::format( "{}", e_lua_type::nil ) == "nil" );
  REQUIRE( fmt::format( "{}", e_lua_type::boolean ) ==
           "boolean" );
  REQUIRE( fmt::format( "{}", e_lua_type::lightuserdata ) ==
           "lightuserdata" );
  REQUIRE( fmt::format( "{}", e_lua_type::number ) == "number" );
  REQUIRE( fmt::format( "{}", e_lua_type::string ) == "string" );
  REQUIRE( fmt::format( "{}", e_lua_type::table ) == "table" );
  REQUIRE( fmt::format( "{}", e_lua_type::function ) ==
           "function" );
  REQUIRE( fmt::format( "{}", e_lua_type::userdata ) ==
           "userdata" );
  REQUIRE( fmt::format( "{}", e_lua_type::thread ) == "thread" );
}

TEST_CASE( "[lua-c-api] to_str e_lua_type" ) {
  c_api st;
  auto  to_str_ = []( e_lua_type type ) {
    string res;
    to_str( type, res );
    return res;
  };
  REQUIRE( to_str_( e_lua_type::nil ) == "nil" );
  REQUIRE( to_str_( e_lua_type::boolean ) == "boolean" );
  REQUIRE( to_str_( e_lua_type::lightuserdata ) ==
           "lightuserdata" );
  REQUIRE( to_str_( e_lua_type::number ) == "number" );
  REQUIRE( to_str_( e_lua_type::string ) == "string" );
  REQUIRE( to_str_( e_lua_type::table ) == "table" );
  REQUIRE( to_str_( e_lua_type::function ) == "function" );
  REQUIRE( to_str_( e_lua_type::userdata ) == "userdata" );
  REQUIRE( to_str_( e_lua_type::thread ) == "thread" );
}

TEST_CASE( "[lua-c-api] type_name" ) {
  c_api st;
  REQUIRE( st.type_name( e_lua_type::nil ) == string( "nil" ) );
  REQUIRE( st.type_name( e_lua_type::boolean ) ==
           string( "boolean" ) );
  // Confirmed that the Lua 5.3 implementation uses "userdata"
  // also for light userdata, not sure wy...
  REQUIRE( st.type_name( e_lua_type::lightuserdata ) ==
           string( "userdata" ) );
  REQUIRE( st.type_name( e_lua_type::number ) ==
           string( "number" ) );
  REQUIRE( st.type_name( e_lua_type::string ) ==
           string( "string" ) );
  REQUIRE( st.type_name( e_lua_type::table ) ==
           string( "table" ) );
  REQUIRE( st.type_name( e_lua_type::function ) ==
           string( "function" ) );
  REQUIRE( st.type_name( e_lua_type::userdata ) ==
           string( "userdata" ) );
  REQUIRE( st.type_name( e_lua_type::thread ) ==
           string( "thread" ) );
}

TEST_CASE( "[lua-c-api] push, pop, get, and type_of" ) {
  c_api st;

  SECTION( "nil" ) {
    REQUIRE( st.stack_size() == 0 );
    st.push( nil );
    REQUIRE( st.stack_size() == 1 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::nil );
    REQUIRE( st.get<bool>( -1 ) == false );
    REQUIRE( st.get<int>( -1 ) == nothing );
    REQUIRE( st.get<double>( -1 ) == nothing );
    REQUIRE( st.get<string>( -1 ) == nothing );
    st.pop();
  }
  SECTION( "bool" ) {
    REQUIRE( st.stack_size() == 0 );
    st.push( false );
    REQUIRE( st.stack_size() == 1 );
    st.push( true );
    REQUIRE( st.stack_size() == 2 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::boolean );
    REQUIRE( st.type_of( -2 ) == e_lua_type::boolean );
    REQUIRE( st.get<bool>( -1 ) == true );
    REQUIRE( st.get<bool>( -2 ) == false );
    REQUIRE( st.get<int>( -1 ) == nothing );
    REQUIRE( st.get<double>( -1 ) == nothing );
    REQUIRE( st.get<string>( -1 ) == nothing );
    st.pop();
    st.pop();
  }
  SECTION( "integer" ) {
    REQUIRE( st.stack_size() == 0 );
    st.push( int( 0 ) );
    REQUIRE( st.stack_size() == 1 );
    st.push( 9LL );
    REQUIRE( st.stack_size() == 2 );
    st.push( 7L );
    REQUIRE( st.stack_size() == 3 );
    st.push( 5 );
    REQUIRE( st.stack_size() == 4 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::number );
    REQUIRE( st.type_of( -2 ) == e_lua_type::number );
    REQUIRE( st.type_of( -3 ) == e_lua_type::number );
    REQUIRE( st.type_of( -4 ) == e_lua_type::number );
    REQUIRE( st.get<bool>( -1 ) == true );
    REQUIRE( st.get<bool>( -2 ) == true );
    REQUIRE( st.get<bool>( -3 ) == true );
    REQUIRE( st.get<bool>( -4 ) == true );
    REQUIRE( st.get<int>( -1 ) == 5 );
    REQUIRE( st.get<int>( -2 ) == 7 );
    REQUIRE( st.get<int>( -3 ) == 9 );
    REQUIRE( st.get<int>( -4 ) == 0 );
    REQUIRE( st.get<double>( -1 ) == 5.0 );
    REQUIRE( st.get<double>( -2 ) == 7.0 );
    REQUIRE( st.get<string>( -1 ) == "5" );
    REQUIRE( st.get<string>( -2 ) == "7" );
    REQUIRE( st.get<string>( -3 ) == "9" );
    REQUIRE( st.get<string>( -4 ) == "0" );
    // Lua changes the value on the stack when we convert to a
    // string.
    REQUIRE( st.type_of( -1 ) == e_lua_type::string );
    REQUIRE( st.type_of( -2 ) == e_lua_type::string );
    REQUIRE( st.type_of( -3 ) == e_lua_type::string );
    REQUIRE( st.type_of( -4 ) == e_lua_type::string );
    st.pop();
    st.pop();
    st.pop();
    st.pop();
  }
  SECTION( "floating" ) {
    REQUIRE( st.stack_size() == 0 );
    st.push( 7.1 );
    REQUIRE( st.stack_size() == 1 );
    st.push( 5.0f );
    REQUIRE( st.stack_size() == 2 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::number );
    REQUIRE( st.type_of( -2 ) == e_lua_type::number );
    REQUIRE( st.get<bool>( -1 ) == true );
    REQUIRE( st.get<bool>( -2 ) == true );
    REQUIRE( st.get<int>( -1 ) == 5 );
    // No rounding.
    REQUIRE( st.get<int>( -2 ) == nothing );
    REQUIRE( st.get<double>( -1 ) == 5.0 );
    REQUIRE( st.get<double>( -2 ) == 7.1 );
    REQUIRE( st.get<string>( -1 ) == "5.0" );
    REQUIRE( st.get<string>( -2 ) == "7.1" );
    // Lua changes the value on the stack when we convert to a
    // string.
    REQUIRE( st.type_of( -1 ) == e_lua_type::string );
    REQUIRE( st.type_of( -2 ) == e_lua_type::string );
    st.pop();
    st.pop();
  }
  SECTION( "string" ) {
    REQUIRE( st.stack_size() == 0 );
    st.push( string( "5" ) );
    REQUIRE( st.stack_size() == 1 );
    st.push( "hello" );
    REQUIRE( st.stack_size() == 2 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::string );
    REQUIRE( st.type_of( -2 ) == e_lua_type::string );
    REQUIRE( st.get<bool>( -1 ) == true );
    REQUIRE( st.get<bool>( -2 ) == true );
    REQUIRE( st.get<int>( -1 ) == nothing );
    REQUIRE( st.get<int>( -2 ) == 5 );
    REQUIRE( st.get<double>( -1 ) == nothing );
    REQUIRE( st.get<double>( -2 ) == 5.0 );
    REQUIRE( st.get<string>( -1 ) == "hello" );
    REQUIRE( st.get<string>( -2 ) == "5" );
    st.pop();
    st.pop();
  }
}

TEST_CASE( "[lua-c-api] call" ) {
  c_api st;
  st.openlibs();

  SECTION( "no args, no results" ) {
    char const* lua_script = R"(
      function foo() end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    st.call( /*nargs=*/0, /*nresults=*/0 );
    REQUIRE( st.stack_size() == 0 );
  }

  SECTION( "no args, one result" ) {
    char const* lua_script = R"(
      function foo()
        return 42
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    st.call( /*nargs=*/0, /*nresults=*/1 );
    REQUIRE( st.stack_size() == 1 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::number );
    REQUIRE( st.get<int>( -1 ) == 42 );
    st.pop();
  }

  SECTION( "no args, two results" ) {
    char const* lua_script = R"(
      function foo()
        return 42, "hello"
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    st.call( /*nargs=*/0, /*nresults=*/2 );
    REQUIRE( st.stack_size() == 2 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::string );
    REQUIRE( st.type_of( -2 ) == e_lua_type::number );
    REQUIRE( st.get<string>( -1 ) == "hello" );
    REQUIRE( st.get<int>( -2 ) == 42 );
    st.pop();
    st.pop();
  }

  SECTION( "no args, LUA_MULTRET" ) {
    char const* lua_script = R"(
      function foo()
        return 42, "hello", "world"
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    st.call( /*nargs=*/0, /*nresults=*/LUA_MULTRET );
    REQUIRE( st.stack_size() == 3 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::string );
    REQUIRE( st.type_of( -2 ) == e_lua_type::string );
    REQUIRE( st.type_of( -3 ) == e_lua_type::number );
    REQUIRE( st.get<string>( -1 ) == "world" );
    REQUIRE( st.get<string>( -2 ) == "hello" );
    REQUIRE( st.get<int>( -3 ) == 42 );
    st.pop();
    st.pop();
    st.pop();
  }

  SECTION( "one arg, no results" ) {
    char const* lua_script = R"(
      function foo( n )
        assert( n == 5 )
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    st.push( 5 );
    REQUIRE( st.stack_size() == 2 );
    st.call( /*nargs=*/1, /*nresults=*/0 );
    REQUIRE( st.stack_size() == 0 );
  }

  SECTION( "two args, no results" ) {
    char const* lua_script = R"(
      function foo( n, s )
        assert( n == 5 )
        assert( s == "hello" )
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    st.push( 5 );
    st.push( "hello" );
    REQUIRE( st.stack_size() == 3 );
    st.call( /*nargs=*/2, /*nresults=*/0 );
    REQUIRE( st.stack_size() == 0 );
  }

  SECTION( "two args, two results" ) {
    char const* lua_script = R"(
      function foo( n, s )
        assert( n == 42 )
        assert( s == "hello" )
        return n+1, s .. " world"
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    st.push( 42 );
    st.push( "hello" );
    REQUIRE( st.stack_size() == 3 );
    st.call( /*nargs=*/2, /*nresults=*/2 );
    REQUIRE( st.stack_size() == 2 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::string );
    REQUIRE( st.type_of( -2 ) == e_lua_type::number );
    REQUIRE( st.get<string>( -1 ) == "hello world" );
    REQUIRE( st.get<double>( -2 ) == 43 );
    st.pop();
    st.pop();
  }
}

TEST_CASE( "[lua-c-api] pcall" ) {
  c_api st;
  st.openlibs();

  SECTION( "no args, no results" ) {
    char const* lua_script = R"(
      function foo() end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    REQUIRE( st.pcall( /*nargs=*/0, /*nresults=*/0 ) == valid );
    REQUIRE( st.stack_size() == 0 );
  }

  SECTION( "no args, one result" ) {
    char const* lua_script = R"(
      function foo()
        return 42
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    REQUIRE( st.pcall( /*nargs=*/0, /*nresults=*/1 ) == valid );
    REQUIRE( st.stack_size() == 1 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::number );
    REQUIRE( st.get<int>( -1 ) == 42 );
    st.pop();
  }

  SECTION( "no args, two results" ) {
    char const* lua_script = R"(
      function foo()
        return 42, "hello"
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    REQUIRE( st.pcall( /*nargs=*/0, /*nresults=*/2 ) == valid );
    REQUIRE( st.stack_size() == 2 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::string );
    REQUIRE( st.type_of( -2 ) == e_lua_type::number );
    REQUIRE( st.get<string>( -1 ) == "hello" );
    REQUIRE( st.get<int>( -2 ) == 42 );
    st.pop();
    st.pop();
  }

  SECTION( "no args, LUA_MULTRET" ) {
    char const* lua_script = R"(
      function foo()
        return 42, "hello", "world"
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    REQUIRE( st.pcall( /*nargs=*/0, /*nresults=*/LUA_MULTRET ) ==
             valid );
    REQUIRE( st.stack_size() == 3 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::string );
    REQUIRE( st.type_of( -2 ) == e_lua_type::string );
    REQUIRE( st.type_of( -3 ) == e_lua_type::number );
    REQUIRE( st.get<string>( -1 ) == "world" );
    REQUIRE( st.get<string>( -2 ) == "hello" );
    REQUIRE( st.get<int>( -3 ) == 42 );
    st.pop();
    st.pop();
    st.pop();
  }

  SECTION( "one arg, no results" ) {
    char const* lua_script = R"(
      function foo( n )
        assert( n == 5 )
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    st.push( 5 );
    REQUIRE( st.stack_size() == 2 );
    REQUIRE( st.pcall( /*nargs=*/1, /*nresults=*/0 ) == valid );
    REQUIRE( st.stack_size() == 0 );
  }

  SECTION( "one arg, error" ) {
    char const* lua_script = R"(
      function foo( n )
        assert( n == 5 )
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    st.push( 6 );
    REQUIRE( st.stack_size() == 2 );
    // clang-format off
    char const* err =
      "[string \"...\"]:3: assertion failed!"  "\n"
      "stack traceback:"                       "\n"
      "\t[C]: in function 'assert'"            "\n"
      "\t[string \"...\"]:3: in function 'foo'";
    // clang-format on
    REQUIRE( st.pcall( /*nargs=*/1, /*nresults=*/0 ) ==
             lua_invalid( err ) );
    REQUIRE( st.stack_size() == 0 );
  }

  SECTION( "two args, no results" ) {
    char const* lua_script = R"(
      function foo( n, s )
        assert( n == 5 )
        assert( s == "hello" )
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    st.push( 5 );
    st.push( "hello" );
    REQUIRE( st.stack_size() == 3 );
    REQUIRE( st.pcall( /*nargs=*/2, /*nresults=*/0 ) == valid );
    REQUIRE( st.stack_size() == 0 );
  }

  SECTION( "two args, two results" ) {
    char const* lua_script = R"(
      function foo( n, s )
        assert( n == 42 )
        assert( s == "hello" )
        return n+1, s .. " world"
      end
    )";
    REQUIRE( st.dostring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "foo" ) == e_lua_type::function );
    REQUIRE( st.stack_size() == 1 );
    st.push( 42 );
    st.push( "hello" );
    REQUIRE( st.stack_size() == 3 );
    REQUIRE( st.pcall( /*nargs=*/2, /*nresults=*/2 ) == valid );
    REQUIRE( st.stack_size() == 2 );
    REQUIRE( st.type_of( -1 ) == e_lua_type::string );
    REQUIRE( st.type_of( -2 ) == e_lua_type::number );
    REQUIRE( st.get<string>( -1 ) == "hello world" );
    REQUIRE( st.get<double>( -2 ) == 43 );
    st.pop();
    st.pop();
  }
}

TEST_CASE( "[lua-c-api] e_lua_type to string" ) {
  REQUIRE( fmt::format( "{}", e_lua_type::nil ) == "nil" );
  REQUIRE( fmt::format( "{}", e_lua_type::boolean ) ==
           "boolean" );
  REQUIRE( fmt::format( "{}", e_lua_type::lightuserdata ) ==
           "lightuserdata" );
  REQUIRE( fmt::format( "{}", e_lua_type::number ) == "number" );
  REQUIRE( fmt::format( "{}", e_lua_type::string ) == "string" );
  REQUIRE( fmt::format( "{}", e_lua_type::table ) == "table" );
  REQUIRE( fmt::format( "{}", e_lua_type::function ) ==
           "function" );
  REQUIRE( fmt::format( "{}", e_lua_type::userdata ) ==
           "userdata" );
  REQUIRE( fmt::format( "{}", e_lua_type::thread ) == "thread" );
}

TEST_CASE( "[lua-c-api] newtable" ) {
  c_api st;
  REQUIRE( st.stack_size() == 0 );
  st.newtable();
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.type_of( -1 ) == e_lua_type::table );
  st.pop();
}

TEST_CASE(
    "[lua-c-api] push_global_table, settable, getfield, "
    "setfield" ) {
  c_api st;
  REQUIRE( st.getglobal( "hello" ) == e_lua_type::nil );
  st.pop();
  REQUIRE( st.stack_size() == 0 );
  st.pushglobaltable();
  REQUIRE( st.stack_size() == 1 );
  st.push( "hello" );
  st.push( 3.5 );
  REQUIRE( st.stack_size() == 3 );
  st.settable( -3 );
  REQUIRE( st.stack_size() == 1 );
  // At this point, only the global table is still on the stack.
  st.pop();

  REQUIRE( st.getglobal( "hello" ) == e_lua_type::number );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.type_of( -1 ) == e_lua_type::number );
  REQUIRE( st.get<double>( -1 ) == 3.5 );
  st.pop();

  st.pushglobaltable();
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.getfield( -1, "hello" ) == e_lua_type::number );
  REQUIRE( st.stack_size() == 2 );
  REQUIRE( st.get<double>( -1 ) == 3.5 );
  st.pop( 1 );

  // At this point, the global table is on the stack.
  REQUIRE( st.stack_size() == 1 );
  st.push( true );
  st.setfield( -2, "hello" );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.type_of( -1 ) == e_lua_type::table );
  REQUIRE( st.getfield( -1, "hello" ) == e_lua_type::boolean );
  REQUIRE( st.stack_size() == 2 );
  REQUIRE( st.get<bool>( -1 ) == true );
  st.pop( 2 );
}

TEST_CASE( "[lua-c-api] rawgeti, rawseti" ) {
  c_api st;
  st.pushglobaltable();
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.rawgeti( -1, 42 ) == e_lua_type::nil );
  REQUIRE( st.stack_size() == 2 );
  REQUIRE( st.type_of( -1 ) == e_lua_type::nil );
  st.pop();
  REQUIRE( st.stack_size() == 1 );

  st.push( "world" );
  st.rawseti( -2, 42 );
  REQUIRE( st.stack_size() == 1 );

  REQUIRE( st.rawgeti( -1, 42 ) == e_lua_type::string );
  REQUIRE( st.stack_size() == 2 );
  REQUIRE( st.type_of( -1 ) == e_lua_type::string );
  REQUIRE( st.get<string>( -1 ) == "world" );
  st.pop( 2 );
}

TEST_CASE( "[lua-c-api] ref/ref_registry/registry_get/unref" ) {
  c_api st;

  st.push( 5 );
  int r1 = st.ref_registry();
  st.push( "hello" );
  int r2 = st.ref_registry();
  REQUIRE( r1 != r2 );
  REQUIRE( st.stack_size() == 0 );

  REQUIRE( st.registry_get( r1 ) == e_lua_type::number );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.get<int>( -1 ) == 5 );
  st.pop();
  REQUIRE( st.registry_get( r2 ) == e_lua_type::string );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.get<string>( -1 ) == "hello" );
  st.pop();
}

TEST_CASE( "[lua-c-api] len" ) {
  c_api st;

  // Table length.
  REQUIRE( st.dostring( "x = {4,5,6}" ) == valid );
  REQUIRE( st.getglobal( "x" ) == e_lua_type::table );
  REQUIRE( st.len_pop( -1 ) == 3 );
  st.pop();
  REQUIRE( st.stack_size() == 0 );

  // String length.
  REQUIRE( st.dostring( "y = 'hello'" ) == valid );
  REQUIRE( st.getglobal( "y" ) == e_lua_type::string );
  REQUIRE( st.len_pop( -1 ) == 5 );
  st.pop();
  REQUIRE( st.stack_size() == 0 );
}

TEST_CASE( "[lua-c-api] geti" ) {
  c_api st;

  REQUIRE(
      st.dostring( "x = {[4]='one',[5]='two',[6]='three'}" ) ==
      valid );
  REQUIRE( st.getglobal( "x" ) == e_lua_type::table );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.geti( -1, 5 ) == e_lua_type::string );
  REQUIRE( st.stack_size() == 2 );
  REQUIRE( st.get<string>( -1 ) == "two" );
  st.pop( 2 );
  REQUIRE( st.stack_size() == 0 );
}

TEST_CASE( "[lua-c-api] push c function" ) {
  c_api st;
  st.openlibs();

  st.push( []( lua_State* L ) -> int {
    int n = luaL_checkinteger( L, 1 );
    lua_pushinteger( L, n + 3 );
    return 1;
  } );
  st.setglobal( "bar" );

  REQUIRE( st.dostring( R"(
    local input  = 7
    local expect = 10
    local output    = bar( input )
    assert( output == expect,
            tostring( output ) .. ' != ' .. tostring( expect ) )
  )" ) == valid );
}

TEST_CASE(
    "[lua-c-api] push c function with upvalues + getupvalue" ) {
  c_api st;
  st.openlibs();

  REQUIRE( st.stack_size() == 0 );
  st.push( 42 );
  st.push(
      []( lua_State* L ) -> int {
        int n = luaL_checkinteger( L, 1 );
        int upvalue =
            luaL_checkinteger( L, lua_upvalueindex( 1 ) );
        lua_pushinteger( L, n + upvalue );
        return 1;
      },
      /*upvalues=*/1 );
  // Consumes the upvalue but leaves the closure on the stack.
  REQUIRE( st.stack_size() == 1 );
  st.setglobal( "bar" );

  REQUIRE( st.dostring( R"(
    local input  = 7
    local expect = 49 -- 7+42
    local output    = bar( input )
    assert( output == expect,
            tostring( output ) .. ' != ' .. tostring( expect ) )
  )" ) == valid );

  // Test that the function has an up value and that the upvalue
  // has the right type.
  st.getglobal( "bar" );
  REQUIRE( st.type_of( -1 ) == e_lua_type::function );
  REQUIRE_FALSE( st.getupvalue( -1, 2 ) );
  REQUIRE( st.getupvalue( -1, 1 ) == true );
  REQUIRE( st.type_of( -1 ) == e_lua_type::number );
  REQUIRE( st.stack_size() == 2 );
  REQUIRE( st.get<int>( -1 ) == 42 );
  st.pop( 2 );
  REQUIRE( st.stack_size() == 0 );
}

TEST_CASE( "[lua-c-api] insert" ) {
  c_api st;

  REQUIRE( st.stack_size() == 0 );
  st.push( 5 );
  st.push( "hello" );
  st.push( true );
  st.push( 7 );
  REQUIRE( st.type_of( -1 ) == e_lua_type::number );
  REQUIRE( st.type_of( -2 ) == e_lua_type::boolean );
  REQUIRE( st.type_of( -3 ) == e_lua_type::string );
  REQUIRE( st.type_of( -4 ) == e_lua_type::number );

  st.insert( -3 );
  REQUIRE( st.type_of( -1 ) == e_lua_type::boolean );
  REQUIRE( st.type_of( -2 ) == e_lua_type::string );
  REQUIRE( st.type_of( -3 ) == e_lua_type::number );
  REQUIRE( st.type_of( -4 ) == e_lua_type::number );
  REQUIRE( st.get<int>( -3 ) == 7 );
  REQUIRE( st.get<int>( -4 ) == 5 );
}

TEST_CASE( "[lua-c-api] swap_top" ) {
  c_api st;

  REQUIRE( st.stack_size() == 0 );
  st.push( 5 );
  st.push( "hello" );
  st.push( true );
  st.push( 7 );
  REQUIRE( st.type_of( -1 ) == e_lua_type::number );
  REQUIRE( st.type_of( -2 ) == e_lua_type::boolean );
  REQUIRE( st.type_of( -3 ) == e_lua_type::string );
  REQUIRE( st.type_of( -4 ) == e_lua_type::number );

  st.swap_top();
  REQUIRE( st.type_of( -1 ) == e_lua_type::boolean );
  REQUIRE( st.type_of( -2 ) == e_lua_type::number );
  REQUIRE( st.type_of( -3 ) == e_lua_type::string );
  REQUIRE( st.type_of( -4 ) == e_lua_type::number );
  REQUIRE( st.get<int>( -2 ) == 7 );
}

TEST_CASE( "[lua-c-api] setmetatable/getmetatable" ) {
  c_api st;
  st.openlibs();

  st.newtable();
  st.newtable(); // metatable
  REQUIRE( st.stack_size() == 2 );
  st.pushglobaltable();
  REQUIRE( st.stack_size() == 3 );
  st.setfield( -2, "__index" );
  REQUIRE( st.stack_size() == 2 );
  st.setmetatable( -2 );
  REQUIRE( st.stack_size() == 1 );
  st.setglobal( "x" );
  REQUIRE( st.stack_size() == 0 );
  REQUIRE( st.dostring( R"(
    x.assert( x.print ~= nil )
  )" ) == valid );
  REQUIRE( st.stack_size() == 0 );

  st.getglobal( "x" );
  REQUIRE( st.stack_size() == 1 );
  st.getmetatable( -1 );
  REQUIRE( st.stack_size() == 2 );
  st.push( nil );
  REQUIRE( st.stack_size() == 3 );
  st.setfield( -2, "__index" );
  REQUIRE( st.stack_size() == 2 );
  st.pop( 2 );
  REQUIRE( st.stack_size() == 0 );

  // clang-format off
  char const* err =
    "[string \"...\"]:2: attempt to call a nil value (field 'assert')" "\n"
    "stack traceback:"                                                 "\n"
    "\t[string \"...\"]:2: in main chunk";
  // clang-format on
  REQUIRE( st.dostring( R"(
    x.assert( x.print ~= nil )
  )" ) == lua_invalid( err ) );
}

TEST_CASE( "[lua-c-api] newuserdata" ) {
  c_api st;
  void* p = st.newuserdata( 1 );
  REQUIRE( st.type_of( -1 ) == e_lua_type::userdata );
  REQUIRE( st.get<void*>( -1 ) == p );
  st.pop();
}

TEST_CASE(
    "[lua-c-api] udata_{new,get,set}metatable + "
    "{check,test}udata" ) {
  c_api st;

  REQUIRE( st.udata_getmetatable( "hello" ) == e_lua_type::nil );
  REQUIRE( st.stack_size() == 1 );
  st.pop();
  REQUIRE( st.udata_newmetatable( "hello" ) == true );
  REQUIRE( st.stack_size() == 1 );
  st.pop();
  REQUIRE( st.udata_newmetatable( "hello" ) == false );
  REQUIRE( st.stack_size() == 1 );
  st.pop();
  REQUIRE( st.udata_getmetatable( "hello" ) ==
           e_lua_type::table );
  REQUIRE( st.stack_size() == 1 );
  st.getfield( -1, "__name" );
  REQUIRE( st.type_of( -1 ) == e_lua_type::string );
  REQUIRE( st.get<string>( -1 ) == "hello" );
  st.pop( 2 );
  REQUIRE( st.stack_size() == 0 );

  REQUIRE( st.newuserdata( 1 ) != nullptr );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.testudata( -1, "hello" ) == nullptr );
  REQUIRE( st.stack_size() == 1 );
  st.udata_setmetatable( "hello" );
  REQUIRE( st.stack_size() == 1 );

  REQUIRE( st.testudata( -1, "hello" ) != nullptr );
}

TEST_CASE( "[lua-c-api] error" ) {
  c_api st;

  SECTION( "error push" ) {
    st.push( []( lua_State* L ) -> int {
      c_api C( L, /*own=*/false );
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
    REQUIRE( st.pcall( 0, 0 ) == lua_invalid( err ) );
  }

  SECTION( "error arg" ) {
    st.push( []( lua_State* L ) -> int {
      c_api C( L, /*own=*/false );
      C.error( "this is an error." );
      return 0;
    } );
    // clang-format off
    char const* err =
      "this is an error.\n"
      "stack traceback:\n"
      "\t[C]: in ?";
    // clang-format on
    REQUIRE( st.pcall( 0, 0 ) == lua_invalid( err ) );
  }
}

TEST_CASE( "[lua-c-api] constants" ) {
  REQUIRE( c_api::noref() == LUA_NOREF );
  REQUIRE( c_api::multret() == LUA_MULTRET );
}

TEST_CASE( "[lua-c-api] pushvalue" ) {
  c_api st;
  st.push( 5 );
  REQUIRE( st.stack_size() == 1 );
  st.pushvalue( -1 );
  REQUIRE( st.stack_size() == 2 );
  REQUIRE( st.get<int>( -1 ) == 5 );
  st.push( 7 );
  REQUIRE( st.stack_size() == 3 );
  REQUIRE( st.get<int>( -1 ) == 7 );
  st.pushvalue( -2 );
  REQUIRE( st.stack_size() == 4 );
  REQUIRE( st.get<int>( -1 ) == 5 );
  st.pop( 4 );
  REQUIRE( st.stack_size() == 0 );
}

} // namespace
} // namespace luapp
