/****************************************************************
**any.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-24.
*
* Description: Unit tests for the src/luapp/any.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/any.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/cast.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

LUA_TEST_CASE( "[any] construct from any" ) {
  st["hello"] = st.table.create();

  auto a = any( cast<table>( st["hello"] ) );
  REQUIRE( a == st["hello"] );

  any b = a;
  REQUIRE( b == st["hello"] );
}

} // namespace
} // namespace lua
