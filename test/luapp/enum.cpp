/****************************************************************
**enum.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-23.
*
* Description: Unit tests for the src/luapp/enum.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/enum.hpp"

// Testing
#include "test/luapp/common.hpp"

// Must be last.
#include "test/catch-common.hpp"

using namespace std;

/****************************************************************
** Test Data Types
*****************************************************************/
namespace my_ns {

enum class non_reflected_enum { yes, no };

enum class empty_enum {};

enum class my_enum { red, blue, green };

} // namespace my_ns

namespace refl {

template<>
struct traits<my_ns::empty_enum> {
  using type                        = my_ns::empty_enum;
  static constexpr type_kind   kind = type_kind::enum_kind;
  static constexpr string_view ns   = "my_ns";
  static constexpr string_view name = "empty_enum";

  // Enum specific.
  static constexpr array<string_view, 0> value_names{};
};

template<>
struct traits<my_ns::my_enum> {
  using type                        = my_ns::my_enum;
  static constexpr type_kind   kind = type_kind::enum_kind;
  static constexpr string_view ns   = "my_ns";
  static constexpr string_view name = "my_enum";

  // Enum specific.
  static constexpr array<string_view, 3> value_names{
      "red",
      "blue",
      "green",
  };
};

} // namespace refl

namespace lua {
namespace {

/****************************************************************
** Static Tests
*****************************************************************/
static_assert( Stackable<my_ns::my_enum> );
static_assert( Stackable<my_ns::empty_enum> );
static_assert( !Pushable<my_ns::non_reflected_enum> );
static_assert( !Gettable<my_ns::non_reflected_enum> );

/****************************************************************
** Test Cases
*****************************************************************/
LUA_TEST_CASE( "[luapp/enum] push/get" ) {
  push( L, my_ns::my_enum::green );
  REQUIRE( C.type_of( -1 ) == type::string );
  REQUIRE( C.get<string>( -1 ) == "green" );
  C.pop();

  push( L, my_ns::my_enum::blue );
  REQUIRE( C.type_of( -1 ) == type::string );
  REQUIRE( C.get<string>( -1 ) == "blue" );
  C.pop();

  base::maybe<my_ns::my_enum> e;

  push( L, "green" );
  e = lua::get<my_ns::my_enum>( L, -1 );
  REQUIRE( e == my_ns::my_enum::green );
  C.pop();

  push( L, "red" );
  e = lua::get<my_ns::my_enum>( L, -1 );
  REQUIRE( e == my_ns::my_enum::red );
  C.pop();

  push( L, "blue" );
  e = lua::get<my_ns::my_enum>( L, -1 );
  REQUIRE( e == my_ns::my_enum::blue );
  C.pop();

  push( L, "xxx" );
  e = lua::get<my_ns::my_enum>( L, -1 );
  REQUIRE( e == base::nothing );
}

} // namespace
} // namespace lua
