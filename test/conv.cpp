/****************************************************************
**conv.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-02.
*
* Description: Unit tests for the src/base/conv.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "base/conv.hpp"

// Must be last.
#include "catch-common.hpp"

namespace base {
namespace {

using namespace std;

TEST_CASE( "[conv] from string" ) {
  REQUIRE( !base::stoi( "" ).has_value() );
  REQUIRE( base::stoi( "0" ) == 0 );
  REQUIRE( base::stoi( "1" ) == 1 );
  REQUIRE( base::stoi( "222" ) == 222 );
  REQUIRE( base::stoi( "0", 16 ) == 0 );
  REQUIRE( base::stoi( "10", 16 ) == 16 );
  REQUIRE( base::stoi( "-10" ) == -10 );
  REQUIRE( base::stoi( "-0" ) == 0 );
  REQUIRE( !base::stoi( "1 a" ).has_value() );

  REQUIRE( !base::from_chars<int>( "" ).has_value() );
  REQUIRE( base::from_chars<int>( "0" ) == 0 );
  REQUIRE( base::from_chars<int>( "1" ) == 1 );
  REQUIRE( base::from_chars<int>( "222" ) == 222 );
  REQUIRE( base::from_chars<int>( "0", 16 ) == 0 );
  REQUIRE( base::from_chars<int>( "10", 16 ) == 16 );
  REQUIRE( base::from_chars<int>( "-10" ) == -10 );
  REQUIRE( base::from_chars<int>( "-0" ) == 0 );
  REQUIRE( !base::from_chars<int>( "1 a" ).has_value() );
}

} // namespace
} // namespace base
