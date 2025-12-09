/****************************************************************
**ext-refl-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-30.
*
* Description: Unit tests for the luapp/ext-refl module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/ext-refl.hpp"

// Testing.
#include "test/luapp/common.hpp"
#include "test/rds/testing.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace lua {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
// Here we are testing both the variant and struct at once be-
// cause the variant's members are reflected structs.
LUA_TEST_CASE(
    "[luapp/ext-refl] Reflected Variant/Struct API" ) {
  using ::rn::MySumtype;
  /*
   sumtype.MySumtype {
    none {},
    some {
      s 'std::string',
      y 'int',
    },
    more {
      d 'double',
    },
  }
  */

  st.lib.open_all();
  define_usertype_for( st, tag<MySumtype>{} );
  define_usertype_for( st, tag<MySumtype::none>{} );
  define_usertype_for( st, tag<MySumtype::some>{} );
  define_usertype_for( st, tag<MySumtype::more>{} );

  auto constexpr script   = R"lua(
    local s = ...
    assert( s )
    assert( s.xyz == nil )
    assert( not pcall( function()
      s.select_xyz()
    end ) )
    local some = s.some
    assert( some.s == "hello" )
    assert( some.y == 5 )
    local more = s.select_more()
    more.d = 4.5
    assert( s.more.d == 4.5 )
    return 42
  )lua";
  lua::rfunction const fn = st.script.load( script );

  MySumtype s = MySumtype::some{ .s = "hello", .y = 5 };

  REQUIRE( fn.pcall<int>( s ) == 42 );

  REQUIRE( s == MySumtype::more{ .d = 4.5 } );
}

} // namespace
} // namespace lua
