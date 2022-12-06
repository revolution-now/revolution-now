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
#include "test/testing.hpp"

// Under test.
#include "base/variant.hpp"
#include "src/variant.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

template<typename... Args>
using V = ::base::variant<Args...>;

TEST_CASE( "[variant] visitation" ) {
  V<int, double> v = 4.4;
  auto f = []( auto&& _ ) { return fmt::format( "{}", _ ); };
  REQUIRE( base::visit( f, v ) == "4.4" );

  // FIXME: remove this guard once libc++ adds the C++20
  // std::visit<R> overload.
#if !defined( _LIBCPP_VERSION )
  v = 3;
  REQUIRE( base::visit<string>( f, v ) == "3" );
#endif
}

TEST_CASE( "[variant] visit member" ) {
  V<int, double> v = 4.4;
  auto f = []( auto&& _ ) { return fmt::format( "{}", _ ); };
  REQUIRE( v.visit( f ) == "4.4" );

  // FIXME: remove this guard once libc++ adds the C++20
  // std::visit<R> overload.
#if !defined( _LIBCPP_VERSION )
  v = 3;
  REQUIRE( v.visit<string>( f ) == "3" );
#endif
}

TEST_CASE( "[variant] get_if" ) {
  V<int, double> v = 4.4;
  REQUIRE( v.get_if<int>() == nothing );
  REQUIRE( v.get_if<double>() == 4.4 );
  static_assert( is_same_v<decltype( v.get_if<double>() ),
                           maybe<double&>> );

  v = 3;
  REQUIRE( v.get_if<int>() == 3 );
  REQUIRE( v.get_if<double>() == nothing );
}

TEST_CASE( "[variant] fmt" ) {
  V<int, double> v = 4.4;
  REQUIRE( fmt::format( "{}", v ) == "4.4" );
  v = 3;
  REQUIRE( fmt::format( "{}", v ) == "3" );
}

TEST_CASE( "[variant] holds" ) {
  V<int, string> v1{ 5 };
  V<int, string> v2{ "hello" };

  REQUIRE( ::rn::holds<int>( v1 ) );
  REQUIRE( !::rn::holds<string>( v1 ) );
  REQUIRE( ::rn::holds<string>( v2 ) );
  REQUIRE( !::rn::holds<int>( v2 ) );
  REQUIRE( ::rn::holds( v1, 5 ) );
  REQUIRE( !::rn::holds( v1, 6 ) );
  REQUIRE( !::rn::holds( v1, string( "world" ) ) );
}

TEST_CASE( "[variant] if_get" ) {
  V<int, string> v1{ 5 };
  V<int, string> v2{ "hello" };

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

enum class e_test_enum { one, two, three };

} // namespace

template<>
struct variant_to_enum<V<int, float, string>> {
  using type = e_test_enum;
};

namespace {

TEST_CASE( "[variant] to_enum" ) {
  V<int, float, std::string> v;
  static_assert(
      is_same_v<decltype( v.to_enum() ), e_test_enum> );
  v = "hello";
  REQUIRE( v.to_enum() == e_test_enum::three );
  v = 3.3f;
  REQUIRE( v.to_enum() == e_test_enum::two );
}

TEST_CASE( "[variant] get" ) {
  V<int, float, std::string> v;
  v = 3;
  REQUIRE( v.get<int>() == 3 );
  v = 3.3f;
  REQUIRE( v.get<float>() == 3.3f );
  v = "hello"s;
  REQUIRE( v.get<string>() == "hello" );
}

} // namespace
} // namespace base