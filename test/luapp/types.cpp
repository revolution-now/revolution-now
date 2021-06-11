/****************************************************************
**types.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-04.
*
* Description: Unit tests for the src/luapp/types.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/types.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/c-api.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::e_lua_type );

namespace lua {
namespace {

using namespace std;

LUA_TEST_CASE( "[types] push" ) {
  push( L, nil );
  REQUIRE( C.type_of( -1 ) == e_lua_type::nil );

  push( L, 5 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::number );
  REQUIRE( C.isinteger( -1 ) );
  REQUIRE( C.get<int>( -1 ) == 5 );

  push( L, 5.0 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::number );
  REQUIRE_FALSE( C.isinteger( -1 ) );
  REQUIRE( C.get<double>( -1 ) == 5.0 );

  push( L, 5.5 );
  REQUIRE( C.type_of( -1 ) == e_lua_type::number );
  REQUIRE_FALSE( C.isinteger( -1 ) );
  REQUIRE( C.get<double>( -1 ) == 5.5 );

  push( L, true );
  REQUIRE( C.type_of( -1 ) == e_lua_type::boolean );
  REQUIRE( C.get<bool>( -1 ) == true );

  push( L, (void*)L );
  REQUIRE( C.type_of( -1 ) == e_lua_type::lightuserdata );
  REQUIRE( C.get<void*>( -1 ) == L );

  push( L, "hello" );
  REQUIRE( C.type_of( -1 ) == e_lua_type::string );
  REQUIRE( C.get<string>( -1 ) == "hello" );
}

} // namespace
} // namespace lua
