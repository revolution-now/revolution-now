/****************************************************************
**ext-base.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-02.
*
* Description: Unit tests for the src/cdr/ext-base.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/cdr/ext-base.hpp"

// cdr
#include "src/cdr/converter.hpp"
#include "src/cdr/ext-builtin.hpp"
#include "src/cdr/ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace cdr {
namespace {

using namespace std;
using namespace base;

using ::cdr::testing::conv_from_bt;

converter conv;

TEST_CASE( "[ext-base] maybe" ) {
  static_assert( Canonical<maybe<int>> );
  static_assert( Canonical<maybe<string>> );
  // We don't want maybe-refs to be canonicalizable since if
  // we're trying to do that it is probably a sign of a bug.
  static_assert( !FromCanonical<maybe<string const&>> );
  static_assert( !FromCanonical<maybe<string&>> );
  static_assert( !FromCanonical<maybe<int const&>> );
  static_assert( !FromCanonical<maybe<int&>> );
  SECTION( "to_canonical" ) {
    maybe<string> m;
    REQUIRE( conv.to( m ) == null );
    m = "hello";
    REQUIRE( conv.to( m ) == string( "hello" ) );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<maybe<int>>( conv, 5 ) == 5 );
    REQUIRE( conv_from_bt<maybe<int>>( conv, null ) == nothing );
    REQUIRE( conv_from_bt<maybe<string>>( conv, "5" ) == "5" );
    REQUIRE( conv_from_bt<maybe<string>>( conv, null ) ==
             nothing );
    REQUIRE( conv.from<maybe<string>>( 5 ) ==
             conv.err( "expected type string, instead found "
                       "type integer." ) );
  }
}

TEST_CASE( "[ext-base] heap_value" ) {
  using ::base::heap_value;
  static_assert( Canonical<heap_value<int>> );
  static_assert( Canonical<heap_value<string>> );
  SECTION( "to_canonical" ) {
    heap_value<string> m = "hello";
    REQUIRE( conv.to( m ) == string( "hello" ) );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<heap_value<int>>( conv, 5 ) == 5 );
    REQUIRE( conv_from_bt<heap_value<string>>( conv, "5" ) ==
             "5" );
    REQUIRE( conv.from<heap_value<string>>( 5 ) ==
             conv.err( "expected type string, instead found "
                       "type integer." ) );
  }
}

TEST_CASE( "[ext-base] base::variant" ) {
  SECTION( "to_canonical" ) {
    static_assert( !ToCanonical<base::variant<int, string>> );
  }
  SECTION( "from_canonical" ) {
    static_assert( !FromCanonical<base::variant<int, string>> );
  }
}

} // namespace
} // namespace cdr
