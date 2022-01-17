/****************************************************************
**heap-value.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-17.
*
* Description: Unit tests for the src/base/heap-value.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/heap-value.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

struct MoveOnly {
  MoveOnly() = default;

  MoveOnly( MoveOnly const& ) = delete;
  MoveOnly( MoveOnly&& )      = default;

  MoveOnly& operator=( MoveOnly const& ) = delete;
  MoveOnly& operator=( MoveOnly&& ) = default;
};

template<typename T>
using HV = heap_value<T>;

struct ForwardDeclared;

using HV_Forward = heap_value<ForwardDeclared>;

static_assert( is_default_constructible_v<HV<int>> );
static_assert( is_copy_constructible_v<HV<int>> );
static_assert( is_copy_assignable_v<HV<int>> );
static_assert( is_move_constructible_v<HV<int>> );
static_assert( is_move_assignable_v<HV<int>> );
static_assert( is_nothrow_move_constructible_v<HV<int>> );
static_assert( is_nothrow_move_assignable_v<HV<int>> );
// static_assert(!std::is_convertible_v<HV<int>, bool>);
static_assert( is_convertible_v<HV<int>, int&> );
static_assert( is_convertible_v<HV<int>, int const&> );
static_assert( !is_convertible_v<HV<int const>, int&> );
static_assert( is_convertible_v<HV<int const>, int const&> );

static_assert( is_default_constructible_v<HV<string>> );
static_assert( is_copy_constructible_v<HV<string>> );
static_assert( is_copy_assignable_v<HV<string>> );
static_assert( is_move_constructible_v<HV<string>> );
static_assert( is_move_assignable_v<HV<string>> );
static_assert( is_nothrow_move_constructible_v<HV<string>> );
static_assert( is_nothrow_move_assignable_v<HV<string>> );
static_assert( !std::is_convertible_v<HV<string>, bool> );

TEST_CASE( "[heap-value] default construction/assignment" ) {
  HV<string> hv;
  REQUIRE( hv == "" );
  REQUIRE( "" == hv );

  hv = "hello";
  REQUIRE( hv == "hello" );
  REQUIRE( "hello" == hv );

  HV<string> hv2 = "world";
  REQUIRE( hv2 == "world" );
  hv = hv2;
  REQUIRE( hv == "world" );
  REQUIRE( hv == hv2 );

  HV<string> hv3( "one" );
  REQUIRE( hv3 == "one" );

  HV<string> hv4;
  REQUIRE( hv4 == "" );
  hv4 = std::move( hv3 );
  REQUIRE( hv3 == "" );
  REQUIRE( hv4 == "one" );

  HV<MoveOnly> hvmo;
  HV<MoveOnly> hvmo2;
  hvmo2 = std::move( hvmo );
}

TEST_CASE( "[heap-value] implicit conversion" ) {
  HV<string> hv = "hello";
  REQUIRE( hv == "hello" );

  string const& s1 = hv;
  REQUIRE( s1 == "hello" );
  string& s2 = hv;
  REQUIRE( s2 == "hello" );
}

TEST_CASE( "[heap-value] copying" ) {
  HV<string> hv1 = "hello";

  HV<string> hv2;
  REQUIRE( hv2 == "" );
  hv2 = hv1;
  REQUIRE( hv2 == "hello" );

  REQUIRE( std::addressof( hv1.get() ) !=
           std::addressof( hv2.get() ) );
}

} // namespace
} // namespace base
