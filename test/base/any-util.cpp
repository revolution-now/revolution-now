/****************************************************************
**any-util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-08.
*
* Description: Unit tests for the src/base/any-util.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/any-util.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

TEST_CASE( "[base/any-util] some test" ) {
  struct X {
    bool operator==( X const& ) const = default;
    int  o                            = 4;
  };
  struct Y {
    bool operator==( Y const& ) const = default;
    int  o                            = 5;
  };
  using V = base::variant<X, Y>;

  V const v = Y{ .o = 3 };
  X const x;
  Y const y;
  any     a;

  a = v;
  REQUIRE( extract_variant_from_any<V>( a ) ==
           V{ Y{ .o = 3 } } );

  a = x;
  REQUIRE( extract_variant_from_any<V>( a ) ==
           V{ X{ .o = 4 } } );

  a = y;
  REQUIRE( extract_variant_from_any<V>( a ) ==
           V{ Y{ .o = 5 } } );
}

} // namespace
} // namespace base
