/****************************************************************
**ext-base.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-26.
*
* Description: Unit tests for the src/rcl/ext-base.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rcl/ext-base.hpp"
#include "src/rcl/ext-builtin.hpp"
#include "src/rcl/ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

using ::base::maybe;
using ::base::nothing;

FMT_TO_CATCH( ::rcl::error );

namespace rcl {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[ext-base] maybe<T> with null" ) {
  value v{ null };

  // Test.
  REQUIRE( convert_to<maybe<string>>( v ) == nothing );
}

TEST_CASE( "[ext-base] maybe<T> with value" ) {
  value v{ string( "hello" ) };

  // Test.
  REQUIRE( convert_to<maybe<string>>( v ) == string( "hello" ) );
}

TEST_CASE( "[ext-base] maybe<T> with type error" ) {
  value v{ 5.5 };

  // Test.
  REQUIRE( convert_to<maybe<string>>( v ) ==
           error( "cannot produce std::string from value of "
                  "type double." ) );
}

} // namespace
} // namespace rcl
