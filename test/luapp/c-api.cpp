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

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::luapp::e_lua_type );

namespace luapp {
namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::base::valid;
using ::Catch::Contains;

string lua_testing_file( string const& filename ) {
  return rn::testing::data_dir() / "lua" / filename;
}

TEST_CASE( "[lua-c-api] create and destroy" ) { c_api st; }

TEST_CASE( "[lua-c-api] openlibs" ) {
  c_api st;
  REQUIRE( st.getglobal( "tostring" ) ==
           lua_unexpected<e_lua_type>(
               "global `tostring` not found." ) );
  REQUIRE( st.stack_size() == 0 );
  st.openlibs();
  REQUIRE( st.getglobal( "tostring" ) == e_lua_type::function );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.enforce_type_of( -1, e_lua_type::function ) ==
           valid );
  st.pop();
  REQUIRE( st.stack_size() == 0 );
}

TEST_CASE( "[lua-c-api] {get,set}global" ) {
  c_api st;
  REQUIRE(
      st.getglobal( "xyz" ) ==
      lua_unexpected<e_lua_type>( "global `xyz` not found." ) );
  REQUIRE( st.stack_size() == 0 );
  REQUIRE( st.dostring( "xyz = 1" ) == valid );
  REQUIRE( st.getglobal( "xyz" ) ==
           lua_expected( e_lua_type::number ) );
  REQUIRE( st.stack_size() == 1 );
  st.pop();
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
    REQUIRE( st.setglobal( "my_module" ) == valid );
    REQUIRE( st.stack_size() == 0 );
    char const* lua_script = R"(
      list = {}
      for i = 1, 5, 1 do
       list[i] = my_module.hello_to_number( i )
      end
    )";
    REQUIRE( st.loadstring( lua_script ) == valid );
    REQUIRE( st.stack_size() == 1 );
    REQUIRE( st.getglobal( "list" ) ==
             lua_unexpected<e_lua_type>(
                 "global `list` not found." ) );
    REQUIRE( st.pcall( /*nargs=*/0, /*nresults=*/0 ) == valid );
    REQUIRE( st.stack_size() == 0 );
    REQUIRE( st.getglobal( "list" ) ==
             lua_expected( e_lua_type::table ) );
    // TODO: test list elements
    st.pop();
    REQUIRE( st.stack_size() == 0 );
  }
}

TEST_CASE( "[lua-c-api] loadstring" ) {
  c_api st;
  REQUIRE(
      st.getglobal( "xyz" ) ==
      lua_unexpected<e_lua_type>( "global `xyz` not found." ) );
  char const* script = "xyz = 1 + 2 + 3";
  REQUIRE( st.loadstring( script ) == valid );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.pcall( /*nargs=*/0, /*nresults=*/0 ) == valid );
  REQUIRE( st.stack_size() == 0 );
  REQUIRE( st.getglobal( "xyz" ) ==
           lua_expected( e_lua_type::number ) );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.enforce_type_of( -1, e_lua_type::number ) ==
           valid );
  st.pop();
  REQUIRE( st.stack_size() == 0 );
}

TEST_CASE( "[lua-c-api] dostring" ) {
  c_api st;
  REQUIRE(
      st.getglobal( "xyz" ) ==
      lua_unexpected<e_lua_type>( "global `xyz` not found." ) );
  char const* script = "xyz = 1 + 2 + 3";
  REQUIRE( st.dostring( script ) == valid );
  REQUIRE( st.stack_size() == 0 );
  REQUIRE( st.getglobal( "xyz" ) ==
           lua_expected( e_lua_type::number ) );
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
  REQUIRE( fmt::format( "{}", e_lua_type::light_userdata ) ==
           "light_userdata" );
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
  REQUIRE( to_str_( e_lua_type::light_userdata ) ==
           "light_userdata" );
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
  REQUIRE( st.type_name( e_lua_type::light_userdata ) ==
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
    REQUIRE( st.get<lua_Integer>( -1 ) == nothing );
    REQUIRE( st.get<lua_Number>( -1 ) == nothing );
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
    REQUIRE( st.get<lua_Integer>( -1 ) == nothing );
    REQUIRE( st.get<lua_Number>( -1 ) == nothing );
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
    REQUIRE( st.get<lua_Integer>( -1 ) == 5 );
    REQUIRE( st.get<lua_Integer>( -2 ) == 7 );
    REQUIRE( st.get<lua_Integer>( -3 ) == 9 );
    REQUIRE( st.get<lua_Integer>( -4 ) == 0 );
    REQUIRE( st.get<lua_Number>( -1 ) == 5.0 );
    REQUIRE( st.get<lua_Number>( -2 ) == 7.0 );
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
    REQUIRE( st.get<lua_Integer>( -1 ) == 5 );
    // No rounding.
    REQUIRE( st.get<lua_Integer>( -2 ) == nothing );
    REQUIRE( st.get<lua_Number>( -1 ) == 5.0 );
    REQUIRE( st.get<lua_Number>( -2 ) == 7.1 );
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
    REQUIRE( st.get<lua_Integer>( -1 ) == nothing );
    REQUIRE( st.get<lua_Integer>( -2 ) == 5 );
    REQUIRE( st.get<lua_Number>( -1 ) == nothing );
    REQUIRE( st.get<lua_Number>( -2 ) == 5.0 );
    REQUIRE( st.get<string>( -1 ) == "hello" );
    REQUIRE( st.get<string>( -2 ) == "5" );
    st.pop();
    st.pop();
  }
}

} // namespace
} // namespace luapp
