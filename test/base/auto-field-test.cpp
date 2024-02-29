/****************************************************************
**auto-field-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-29.
*
* Description: Unit tests for the base/auto-field module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/auto-field.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

#include <tuple>

namespace base {
namespace {

using namespace std;

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
** Static checks.
*****************************************************************/
static_assert( is_same_v<inner_field_type_t<HasOneField>, int> );
static_assert( is_same_v<inner_field_type_t<HasOneField const>,
                         int const> );

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[base/auto-field] single_inner_field" ) {
  HasOneField one_field;
  static_assert(
      is_same_v<decltype( single_inner_field( one_field ) ),
                int&> );
  REQUIRE( single_inner_field( one_field ) == 5 );
  single_inner_field( one_field ) = 6;
  REQUIRE( single_inner_field( one_field ) == 6 );

  HasOneField const one_field_const;
  static_assert( is_same_v<decltype( single_inner_field(
                               one_field_const ) ),
                           int const&> );
}

} // namespace
} // namespace base
