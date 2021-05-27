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

namespace luapp {
namespace {

using namespace std;

using base::valid;
using Catch::Contains;

string lua_testing_file( string const& filename ) {
  return rn::testing::data_dir() / "lua" / filename;
}

TEST_CASE( "[lua-c-api] some test" ) {
  c_api st;
  st.openlibs();
  REQUIRE( st.dofile( lua_testing_file( "test1.lua" ) ) ==
           valid );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.enforce_type_of( -1, e_lua_type::table ) ==
           valid );
  REQUIRE( st.setglobal( "my_module" ) == valid );

  char const* lua_script = R"(
    list = {}
    for i = 1, 5, 1 do
     list[i] = my_module.foo( i )
    end
  )";

  REQUIRE( st.loadstring( lua_script ) == valid );
  REQUIRE( st.stack_size() == 1 );
  REQUIRE( st.pcall( { .nargs = 0, .nresults = 0 } ) == valid );
  REQUIRE( st.stack_size() == 0 );

  REQUIRE( st.getglobal( "listx" ) ==
           lua_unexpected<e_lua_type>(
               "global `listx` not found." ) );

  REQUIRE( st.getglobal( "list" ) ==
           lua_expected( e_lua_type::table ) );
}

} // namespace
} // namespace luapp
