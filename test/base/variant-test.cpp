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
#include "base/variant-util.hpp"
#include "base/variant.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

template<typename... Args>
using V = ::base::variant<Args...>;

/****************************************************************
** Helper data types.
*****************************************************************/
struct HasNoFields {};
struct HasOneField {
  int x = 5;
};
struct HasTwoFields {
  int x = 7;
  int y = 8;
};

/****************************************************************
** Test Cases.
*****************************************************************/
TEST_CASE( "[variant] visitation" ) {
  V<int, double> v = 4.4;
  auto f = []( auto&& _ ) { return fmt::format( "{}", _ ); };
  REQUIRE( std::visit( f, v ) == "4.4" );
  v = 3;
  REQUIRE( std::visit<string>( f, v ) == "3" );
}

TEST_CASE( "[variant] visit member" ) {
  V<int, double> v = 4.4;
  auto f = []( auto&& _ ) { return fmt::format( "{}", _ ); };
  REQUIRE( v.visit( f ) == "4.4" );
  v = 3;
  REQUIRE( v.visit<string>( f ) == "3" );
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

  REQUIRE( ::base::holds<int>( v1 ) );
  REQUIRE( !::base::holds<string>( v1 ) );
  REQUIRE( ::base::holds<string>( v2 ) );
  REQUIRE( !::base::holds<int>( v2 ) );
  REQUIRE( ::base::holds( v1, 5 ) );
  REQUIRE( !::base::holds( v1, 6 ) );
  REQUIRE( !::base::holds( v1, string( "world" ) ) );
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

enum class e_test_enum {
  one,
  two,
  three
};

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

TEST_CASE( "[variant] inner fields" ) {
  V<HasTwoFields, HasOneField, HasNoFields> v = {};
  static_assert( is_same_v<decltype( v.inner_if<HasOneField>() ),
                           maybe<int&>> );
  static_assert(
      is_same_v<
          decltype( as_const( v ).inner_if<HasOneField>() ),
          maybe<int const&>> );

  v = HasTwoFields{};
  REQUIRE( v.inner_if<HasOneField>() == nothing );

  v = HasOneField{ .x = 9 };
  REQUIRE( v.inner_if<HasOneField>() == 9 );

  *v.inner_if<HasOneField>() = 7;
  REQUIRE( v.inner_if<HasOneField>() == 7 );
}

} // namespace
} // namespace base