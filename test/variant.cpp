/****************************************************************
**variant.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-19.
*
* Description: Tests for variant-handling utilities.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "base/variant.hpp"
#include "src/variant.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;

template<typename... Args>
using V = ::base::variant<Args...>;

TEST_CASE( "[base::variant] visitation" ) {
  V<int, double> v = 4.4;
  auto f = []( auto&& _ ) { return fmt::format( "{}", _ ); };
  REQUIRE( std::visit( f, v ) == "4.4" );

  // FIXME: remove this guard once libc++ adds the C++20
  // std::visit<R> overload.
#if !defined( _LIBCPP_VERSION )
  v = 3;
  REQUIRE( std::visit<string>( f, v ) == "3" );
#endif
}

TEST_CASE( "[base::variant] get_if" ) {
  V<int, double> v = 4.4;
  REQUIRE( v.get_if<int>() == nothing );
  REQUIRE( v.get_if<double>() == 4.4 );
  static_assert( is_same_v<decltype( v.get_if<double>() ),
                           maybe<double&>> );

  v = 3;
  REQUIRE( v.get_if<int>() == 3 );
  REQUIRE( v.get_if<double>() == nothing );
}

TEST_CASE( "[base::variant] fmt" ) {
  V<int, double> v = 4.4;
  REQUIRE( fmt::format( "{}", v ) == "4.4" );
  v = 3;
  REQUIRE( fmt::format( "{}", v ) == "3" );
}

TEST_CASE( "[std::variant] holds" ) {
  variant<int, string> v1{ 5 };
  variant<int, string> v2{ "hello" };

  REQUIRE( holds<int>( v1 ) );
  REQUIRE( !holds<string>( v1 ) );
  REQUIRE( holds<string>( v2 ) );
  REQUIRE( !holds<int>( v2 ) );
  REQUIRE( holds( v1, 5 ) );
  REQUIRE( !holds( v1, 6 ) );
  REQUIRE( !holds( v1, string( "world" ) ) );
}

TEST_CASE( "[std::variant] if_get" ) {
  variant<int, string> v1{ 5 };
  variant<int, string> v2{ "hello" };

  bool is_int = false;
  if_get( v2, int, p ) {
    (void)p;
    is_int = true;
  }
  bool is_string = false;
  if_get( v2, string, p ) {
    REQUIRE( p == "hello" );
    is_string = true;
  }

  REQUIRE( !is_int );
  REQUIRE( is_string );
}

} // namespace
} // namespace rn