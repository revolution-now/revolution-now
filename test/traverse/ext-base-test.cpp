/****************************************************************
**ext-base-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-04.
*
* Description: Unit tests for the traverse/ext-base module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/traverse/ext-base.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace trv {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[traverse/ext-base] maybe" ) {
  using T          = base::maybe<string>;
  using K_expected = trv::none_t;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const ) {
    static_assert( is_same_v<K, K_expected> );
    if constexpr( is_same_v<V, string> ) v.push_back( "string" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  v.clear();
  o = base::nothing;
  f();
  REQUIRE( v == vector<string>{} );

  v.clear();
  o = "hello";
  f();
  REQUIRE( v == vector<string>{
                  "string",
                  "hello",
                } );
}

TEST_CASE( "[traverse/ext-base] heap_value" ) {
  using T          = base::heap_value<string>;
  using K_expected = trv::none_t;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const ) {
    static_assert( is_same_v<K, K_expected> );
    if constexpr( is_same_v<V, string> ) v.push_back( "string" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  // Default value of o.
  v.clear();
  f();
  REQUIRE( v == vector<string>{
                  "string",
                  "",
                } );

  v.clear();
  o = "hello";
  f();
  REQUIRE( v == vector<string>{
                  "string",
                  "hello",
                } );
}

} // namespace
} // namespace trv
