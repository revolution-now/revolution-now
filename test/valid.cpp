/****************************************************************
**valid.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-08.
*
* Description: Unit tests for the src/base/valid.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/base/valid.hpp"

// Must be last.
#include "catch-common.hpp"

namespace base {
namespace {

using namespace std;

template<typename E>
using V = ::base::valid_or<E>;

enum class e_code { red, green, blue };

/****************************************************************
** [static]
*****************************************************************/
static_assert( sizeof( V<e_code> ) == 8 );

// No adding new members!
static_assert( sizeof( valid_or<int> ) ==
               sizeof( expect<valid_t, int> ) );

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[valid] valid equality" ) {
  REQUIRE( valid == valid );
}

TEST_CASE( "[valid] construction/copy/move/assignment" ) {
  V<e_code> v1 = e_code::red;
  V<e_code> v2 = e_code::green;
  V<e_code> v3 = valid;
  V<e_code> v4 = e_code::blue;
  V<e_code> v5( std::move( v4 ) );
  V<e_code> v6 = v5;
  v4           = std::move( v1 );
  V<e_code> v7 = invalid<e_code>( e_code::blue );
  V<e_code> v8 = e_code::blue;

  REQUIRE( v3 );
  REQUIRE( v3.valid() );
  REQUIRE( v3 == valid );
  REQUIRE( !v1.valid() );
  REQUIRE( !v1 );
  REQUIRE( v1 == v1 );
  REQUIRE( v1 != v2 );
  REQUIRE( v1 == e_code::red );
  REQUIRE( e_code::red == v1 );
  REQUIRE( v1 != e_code::green );
  REQUIRE( e_code::green != v1 );
  REQUIRE( !v7 );
  REQUIRE( v7.error() == e_code::blue );
  REQUIRE( v7 == v8 );

  v1 = v2;
  REQUIRE( e_code::green == v1 );
}

} // namespace
} // namespace base
