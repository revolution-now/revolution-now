/****************************************************************
**generator.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-16.
*
* Description: Unit tests for the src/generator.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/generator.hpp"

// Must be last.
#include "test/catch-common.hpp"

// C++ standard library
#include <ranges>

namespace base {
namespace {

using namespace std;

static_assert( std::ranges::range<generator<int>> );

generator<int> yields_nothing( bool b = false ) {
  if( b ) co_yield 0;
}

generator<int> yields_one() { co_yield 5; }

generator<int> fibonacci() {
  int n1 = 0;
  int n2 = 1;
  co_yield n1;
  co_yield n2;
  for( ;; ) {
    int const next = n1 + n2;
    co_yield next;
    n1 = n2;
    n2 = next;
  }
}

generator<int> first_n_fibs( int n ) {
  generator<int> fibs = fibonacci();
  auto           iter = fibs.begin();
  for( int i = 0; i < n; ++i ) {
    co_yield *iter;
    ++iter;
  }
}

TEST_CASE( "[generator] yield nothing" ) {
  generator<int> fibs = yields_nothing();
  REQUIRE( fibs.value() == nothing );
  REQUIRE( fibs.begin() == fibs.end() );
}

TEST_CASE( "[generator] yield one" ) {
  generator<int> one = yields_one();
  vector<int>    ns;
  for( int n : one ) ns.push_back( n );
  REQUIRE( ns == vector<int>{ 5 } );
}

TEST_CASE( "[generator] basic generator" ) {
  generator<int> fibs = fibonacci();
  auto           iter = fibs.begin();
  REQUIRE( iter != fibs.end() );
  REQUIRE( *iter == 0 );
  REQUIRE( fibs.value() == 0 );
  ++iter;
  REQUIRE( iter != fibs.end() );
  REQUIRE( *iter == 1 );
  REQUIRE( fibs.value() == 1 );
  ++iter;
  REQUIRE( iter != fibs.end() );
  REQUIRE( *iter == 1 );
  REQUIRE( fibs.value() == 1 );
  ++iter;
  REQUIRE( iter != fibs.end() );
  REQUIRE( *iter == 2 );
  REQUIRE( fibs.value() == 2 );
  ++iter;
  REQUIRE( iter != fibs.end() );
  REQUIRE( *iter == 3 );
  REQUIRE( fibs.value() == 3 );
  ++iter;
  REQUIRE( iter != fibs.end() );
  REQUIRE( *iter == 5 );
  REQUIRE( fibs.value() == 5 );
  ++iter;
  REQUIRE( iter != fibs.end() );
  REQUIRE( *iter == 8 );
  REQUIRE( fibs.value() == 8 );
  ++iter;
  REQUIRE( iter != fibs.end() );
}

TEST_CASE( "[generator] nested generator" ) {
  vector<int> fibs;
  fibs.reserve( 10 );
  for( int n : first_n_fibs( 10 ) ) fibs.push_back( n );
  REQUIRE( fibs ==
           vector<int>{ 0, 1, 1, 2, 3, 5, 8, 13, 21, 34 } );
}

} // namespace
} // namespace base
