/****************************************************************
**metatable.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-24.
*
* Description: Unit tests for the src/luapp/metatable.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/metatable.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/as.hpp"
#include "src/luapp/func-push.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

using ::base::maybe;

LUA_TEST_CASE( "[metatable] metatable" ) {
  st["x"] = st.table.create();
  REQUIRE( st["x"]["y"] == nil );

  table metatable      = st.table.create();
  metatable["__index"] = metatable;
  metatable["y"]       = 5;

  REQUIRE( st["x"]["y"] == nil );
  setmetatable( as<table>( st["x"] ), metatable );
  REQUIRE( st["x"]["y"] == 5 );

  maybe<table> metatable2 = metatable_for( st["x"] );
  REQUIRE( metatable2.has_value() );
  REQUIRE( *metatable2 == metatable );

  metatable["__index"] = []( table, string const& key ) {
    if( key == "yy" ) return 9.9;
    return 1.1;
  };
  REQUIRE( st["x"]["y"] == 1.1 );
  REQUIRE( st["x"]["yy"] == 9.9 );
}

} // namespace
} // namespace lua
