/****************************************************************
**fixed-string.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-04.
*
* Description: Unit tests for the src/base/fixed-string.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/fixed-string.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;
using namespace std::literals::string_view_literals;

/****************************************************************
** fixed_string
*****************************************************************/
static_assert( fixed_string( "" ).ssize() == 0 );
static_assert( fixed_string( "" ).data.size() == 0 );
static_assert( fixed_string( "" ).kArrayLength == 1 );
static_assert( fixed_string( "" ).kStringLength == 0 );

static_assert( fixed_string( "X" ).ssize() == 1 );
static_assert( fixed_string( "X" ).data.size() == 1 );
static_assert( fixed_string( "X" ).kArrayLength == 2 );
static_assert( fixed_string( "X" ).kStringLength == 1 );

static_assert( fixed_string( "hello" ).ssize() == 5 );
static_assert( fixed_string( "hello" ).data.size() == 5 );
static_assert( fixed_string( "hello" ).kArrayLength == 6 );
static_assert( fixed_string( "hello" ).kStringLength == 5 );

static_assert( fixed_string( "" ) == ""sv );
static_assert( fixed_string( "X" ) == "X"sv );
static_assert( fixed_string( "hello" ) == "hello"sv );

/****************************************************************
** parametrized_by_string
*****************************************************************/
template<fixed_string Arr>
struct parametrized_by_string {
  static constexpr size_t size() { return Arr.size(); }

  static constexpr int ssize() { return Arr.ssize(); }

  static constexpr std::string_view sv = Arr;
};

using cts0 = parametrized_by_string<"">;
using cts1 = parametrized_by_string<"X">;
using cts5 = parametrized_by_string<"hello">;

static_assert( is_same_v<cts0, cts0> );
static_assert( is_same_v<cts1, cts1> );
static_assert( is_same_v<cts5, cts5> );

static_assert( !is_same_v<cts0, cts1> );
static_assert( !is_same_v<cts1, cts5> );
static_assert( !is_same_v<cts5, cts0> );

static_assert( cts0::size() == 0 );
static_assert( cts0::ssize() == 0 );
static_assert( cts0::sv == "" );

static_assert( cts1::size() == 1 );
static_assert( cts1::ssize() == 1 );
static_assert( cts1::sv == "X" );

static_assert( cts5::size() == 5 );
static_assert( cts5::ssize() == 5 );
static_assert( cts5::sv == "hello" );

template<fixed_string CS>
constexpr char foo() {
  return string_view( CS )[1];
}

static_assert( foo<"hello">() == 'e' );

/****************************************************************
** fixed_string_holder
*****************************************************************/
static_assert( fixed_string_holder<"hello">::value == "hello" );

/****************************************************************
** UDL
*****************************************************************/
using namespace ::base::literals;

static_assert( decltype( "hello"_t )::value == "hello" );
static_assert( "hello"_t.value == "hello" );

} // namespace
} // namespace base
