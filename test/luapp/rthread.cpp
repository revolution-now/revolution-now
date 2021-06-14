/****************************************************************
**rthread.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: Unit tests for the src/luapp/thread.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/rthread.hpp"

// Testing
#include "test/luapp/common.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

/****************************************************************
** lthread objects
*****************************************************************/
LUA_TEST_CASE( "[lthread] lthread equality" ) {
  boolean       b   = true;
  integer       i   = 1;
  floating      f   = 2.3;
  lightuserdata lud = C.newuserdata( 10 );
  C.pop();

  (void)C.newthread();
  rthread o1( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  (void)C.newthread();
  rthread o2( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  rthread o3 = o1;

  REQUIRE( o1 != o2 );
  REQUIRE( o1 == o3 );

  REQUIRE( o1 != nil );
  REQUIRE( o1 != b );
  REQUIRE( o1 != i );
  REQUIRE( o1 != f );
  REQUIRE( o1 != lud );
  REQUIRE( nil != o1 );
  REQUIRE( b != o1 );
  REQUIRE( i != o1 );
  REQUIRE( f != o1 );
  REQUIRE( lud != o1 );
}

} // namespace
} // namespace lua
