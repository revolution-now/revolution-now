/****************************************************************
**irand.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-19.
*
* Description: Unit tests for the src/irand.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/irand.hpp"

// Testing
#include "test/mocks/irand.hpp"

// ss
#include "src/mock/matchers.hpp"
#include "src/ss/rds-helpers.rds.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::base::nothing;
using ::mock::matchers::Approx;

TEST_CASE( "[irand] shuffle/vector" ) {
  MockIRand impl;
  IRand& irand = impl;

  vector<string> v, expected;

  SECTION( "empty" ) {
    v = {};
    irand.shuffle( v );
    expected = {};
    REQUIRE( v == expected );
  }

  SECTION( "one" ) {
    v = { "hello" };
    irand.shuffle( v );
    expected = { "hello" };
    REQUIRE( v == expected );
  }

  SECTION( "two" ) {
    v = { "hello", "world" };
    impl.EXPECT__uniform_int( 0, 1 ).returns( 0 );
    irand.shuffle( v );
    expected = { "hello", "world" };
    REQUIRE( v == expected );
    impl.EXPECT__uniform_int( 0, 1 ).returns( 1 );
    irand.shuffle( v );
    expected = { "world", "hello" };
    REQUIRE( v == expected );
  }

  SECTION( "three" ) {
    v = { "hello", "world", "again" };
    impl.EXPECT__uniform_int( 0, 2 ).returns( 1 );
    impl.EXPECT__uniform_int( 1, 2 ).returns( 1 );
    irand.shuffle( v );
    expected = { "world", "hello", "again" };
    REQUIRE( v == expected );
  }

  SECTION( "four" ) {
    v = { "hello", "world", "again", "!!" };
    impl.EXPECT__uniform_int( 0, 3 ).returns( 0 );
    impl.EXPECT__uniform_int( 1, 3 ).returns( 2 );
    impl.EXPECT__uniform_int( 2, 3 ).returns( 3 );
    irand.shuffle( v );
    expected = { "hello", "again", "!!", "world" };
    REQUIRE( v == expected );
  }

  SECTION( "four unchanged" ) {
    v = { "hello", "world", "again", "!!" };
    impl.EXPECT__uniform_int( 0, 3 ).returns( 0 );
    impl.EXPECT__uniform_int( 1, 3 ).returns( 1 );
    impl.EXPECT__uniform_int( 2, 3 ).returns( 2 );
    irand.shuffle( v );
    expected = { "hello", "world", "again", "!!" };
    REQUIRE( v == expected );
  }

  SECTION( "four reversed" ) {
    v = { "hello", "world", "again", "!!" };
    impl.EXPECT__uniform_int( 0, 3 ).returns( 3 );
    impl.EXPECT__uniform_int( 1, 3 ).returns( 2 );
    impl.EXPECT__uniform_int( 2, 3 ).returns( 2 );
    irand.shuffle( v );
    expected = { "!!", "again", "world", "hello" };
    REQUIRE( v == expected );
  }
}

TEST_CASE( "[irand] shuffle/array" ) {
  MockIRand impl;
  IRand& irand = impl;

  SECTION( "empty" ) {
    size_t constexpr kSize = 0;
    array<string, kSize> v, expected;
    v = {};
    irand.shuffle( v );
    expected = {};
    REQUIRE( v == expected );
  }

  SECTION( "one" ) {
    size_t constexpr kSize = 1;
    array<string, kSize> v, expected;
    v = { "hello" };
    irand.shuffle( v );
    expected = { "hello" };
    REQUIRE( v == expected );
  }

  SECTION( "two" ) {
    size_t constexpr kSize = 2;
    array<string, kSize> v, expected;
    v = { "hello", "world" };
    impl.EXPECT__uniform_int( 0, 1 ).returns( 0 );
    irand.shuffle( v );
    expected = { "hello", "world" };
    REQUIRE( v == expected );
    impl.EXPECT__uniform_int( 0, 1 ).returns( 1 );
    irand.shuffle( v );
    expected = { "world", "hello" };
    REQUIRE( v == expected );
  }

  SECTION( "three" ) {
    size_t constexpr kSize = 3;
    array<string, kSize> v, expected;
    v = { "hello", "world", "again" };
    impl.EXPECT__uniform_int( 0, 2 ).returns( 1 );
    impl.EXPECT__uniform_int( 1, 2 ).returns( 1 );
    irand.shuffle( v );
    expected = { "world", "hello", "again" };
    REQUIRE( v == expected );
  }

  SECTION( "four" ) {
    size_t constexpr kSize = 4;
    array<string, kSize> v, expected;
    v = { "hello", "world", "again", "!!" };
    impl.EXPECT__uniform_int( 0, 3 ).returns( 0 );
    impl.EXPECT__uniform_int( 1, 3 ).returns( 2 );
    impl.EXPECT__uniform_int( 2, 3 ).returns( 3 );
    irand.shuffle( v );
    expected = { "hello", "again", "!!", "world" };
    REQUIRE( v == expected );
  }

  SECTION( "four unchanged" ) {
    size_t constexpr kSize = 4;
    array<string, kSize> v, expected;
    v = { "hello", "world", "again", "!!" };
    impl.EXPECT__uniform_int( 0, 3 ).returns( 0 );
    impl.EXPECT__uniform_int( 1, 3 ).returns( 1 );
    impl.EXPECT__uniform_int( 2, 3 ).returns( 2 );
    irand.shuffle( v );
    expected = { "hello", "world", "again", "!!" };
    REQUIRE( v == expected );
  }

  SECTION( "four reversed" ) {
    size_t constexpr kSize = 4;
    array<string, kSize> v, expected;
    v = { "hello", "world", "again", "!!" };
    impl.EXPECT__uniform_int( 0, 3 ).returns( 3 );
    impl.EXPECT__uniform_int( 1, 3 ).returns( 2 );
    impl.EXPECT__uniform_int( 2, 3 ).returns( 2 );
    irand.shuffle( v );
    expected = { "!!", "again", "world", "hello" };
    REQUIRE( v == expected );
  }
}

TEST_CASE( "[irand] expect_shuffle" ) {
  MockIRand impl;
  IRand& irand = impl;

  vector<string> to_be_shuffled{ "0", "1", "2", "3", "4",
                                 "5", "6", "7", "8", "9" };
  vector<int> final_indices{ 9, 0, 4, 7, 6, 8, 3, 1, 2, 5 };
  vector<string> expected{ "9", "0", "4", "7", "6",
                           "8", "3", "1", "2", "5" };

  expect_shuffle( impl, final_indices );

  REQUIRE( to_be_shuffled == vector<string>{ "0", "1", "2", "3",
                                             "4", "5", "6", "7",
                                             "8", "9" } );
  irand.shuffle( to_be_shuffled );
  REQUIRE( to_be_shuffled == vector<string>{ "9", "0", "4", "7",
                                             "6", "8", "3", "1",
                                             "2", "5" } );
}

TEST_CASE( "[irand] pick_from_weighted_values<map,int>" ) {
  MockIRand impl;
  IRand& irand = impl;

  map<string, int> weights;

  auto const f = [&] [[clang::noinline]] {
    return irand.pick_from_weighted_values( weights );
  };

  // NOTE: these will be sorted.
  weights = {
    { "hello", 3 },
    { "test", 1 },
    { "world", 5 },
  };
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 0 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 2 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 3 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 4 );
  REQUIRE( f() == "world" );
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 8 );
  REQUIRE( f() == "world" );

  // NOTE: these will be sorted.
  weights = {
    { "hello", 0 },
    { "test", 1 },
    { "world", 0 },
  };
  impl.EXPECT__uniform_int( 0, 1 - 1 ).returns( 0 );
  REQUIRE( f() == "test" );
}

TEST_CASE( "[irand] pick_from_weighted_values<map,double>" ) {
  MockIRand impl;
  IRand& irand = impl;

  map<string, double> weights;

  auto const f = [&] [[clang::noinline]] {
    return irand.pick_from_weighted_values( weights );
  };

  // NOTE: these will be sorted.
  weights = {
    { "hello", 3.5 },
    { "test", 1 },
    { "world", 5 },
  };
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 0 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 3.499999 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 3.5 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 4.499999 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 4.5 );
  REQUIRE( f() == "world" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 9.499999 );
  REQUIRE( f() == "world" );

  // NOTE: these will be sorted.
  weights = {
    { "hello", 0 },
    { "test", 1.1 },
    { "world", 0 },
  };
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 0 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 1 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 1.099999 );
  REQUIRE( f() == "test" );
}

TEST_CASE( "[irand] pick_from_weighted_values<vector,int>" ) {
  MockIRand impl;
  IRand& irand = impl;

  vector<pair<string, int>> weights;

  auto const f = [&] [[clang::noinline]] {
    return irand.pick_from_weighted_values( weights );
  };

  // NOTE: these will be sorted.
  weights = {
    { "hello", 3 },
    { "test", 1 },
    { "world", 5 },
  };
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 0 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 2 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 3 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 4 );
  REQUIRE( f() == "world" );
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 8 );
  REQUIRE( f() == "world" );

  // NOTE: these will be sorted.
  weights = {
    { "hello", 0 },
    { "test", 1 },
    { "world", 0 },
  };
  impl.EXPECT__uniform_int( 0, 1 - 1 ).returns( 0 );
  REQUIRE( f() == "test" );
}

TEST_CASE( "[irand] pick_from_weighted_values<vector,double>" ) {
  MockIRand impl;
  IRand& irand = impl;

  vector<pair<string, double>> weights;

  auto const f = [&] [[clang::noinline]] {
    return irand.pick_from_weighted_values( weights );
  };

  // NOTE: these will be sorted.
  weights = {
    { "hello", 3.5 },
    { "test", 1 },
    { "world", 5 },
  };
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 0 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 3.499999 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 3.5 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 4.499999 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 4.5 );
  REQUIRE( f() == "world" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 9.499999 );
  REQUIRE( f() == "world" );

  // NOTE: these will be sorted.
  weights = {
    { "hello", 0 },
    { "test", 1.1 },
    { "world", 0 },
  };
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 0 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 1 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 1.099999 );
  REQUIRE( f() == "test" );
}

TEST_CASE( "[irand] pick_from_weighted_values_safe<map,int>" ) {
  MockIRand impl;
  IRand& irand = impl;

  map<string, double> weights;

  auto const f = [&] [[clang::noinline]] {
    return irand.pick_from_weighted_values_safe( weights );
  };

  // NOTE: these will be sorted.
  weights = {
    { "hello", 3.5 },
    { "test", 1 },
    { "world", 5 },
  };
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 0 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 3.499999 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 3.5 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 4.499999 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 4.5 );
  REQUIRE( f() == "world" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 9.499999 );
  REQUIRE( f() == "world" );

  // NOTE: these will be sorted.
  weights = {
    { "hello", 0 },
    { "test", 1.1 },
    { "world", 0 },
  };
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 0 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 1 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 1.099999 );
  REQUIRE( f() == "test" );

  weights = {};
  REQUIRE( f() == nothing );

  weights = {
    { "hello", 0 },
    { "test", 0 },
    { "world", 0 },
  };
  REQUIRE( f() == nothing );
}

TEST_CASE(
    "[irand] pick_from_weighted_values_safe<map,double>" ) {
  MockIRand impl;
  IRand& irand = impl;

  map<string, double> weights;

  auto const f = [&] [[clang::noinline]] {
    return irand.pick_from_weighted_values_safe( weights );
  };

  // NOTE: these will be sorted.
  weights = {
    { "hello", 3.5 },
    { "test", 1 },
    { "world", 5 },
  };
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 0 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 3.499999 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 3.5 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 4.499999 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 4.5 );
  REQUIRE( f() == "world" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 9.499999 );
  REQUIRE( f() == "world" );

  // NOTE: these will be sorted.
  weights = {
    { "hello", 0 },
    { "test", 1.1 },
    { "world", 0 },
  };
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 0 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 1 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 1.099999 );
  REQUIRE( f() == "test" );

  weights = {};
  REQUIRE( f() == nothing );

  weights = {
    { "hello", 0 },
    { "test", 0 },
    { "world", 0 },
  };
  REQUIRE( f() == nothing );
}

TEST_CASE(
    "[irand] pick_from_weighted_values_safe<vector,int>" ) {
  MockIRand impl;
  IRand& irand = impl;

  vector<pair<string, int>> weights;

  auto const f = [&] [[clang::noinline]] {
    return irand.pick_from_weighted_values_safe( weights );
  };

  // NOTE: these will be sorted.
  weights = {
    { "hello", 3 },
    { "test", 1 },
    { "world", 5 },
  };
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 0 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 2 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 3 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 4 );
  REQUIRE( f() == "world" );
  impl.EXPECT__uniform_int( 0, 9 - 1 ).returns( 8 );
  REQUIRE( f() == "world" );

  // NOTE: these will be sorted.
  weights = {
    { "hello", 0 },
    { "test", 1 },
    { "world", 0 },
  };
  impl.EXPECT__uniform_int( 0, 1 - 1 ).returns( 0 );
  REQUIRE( f() == "test" );

  weights = {};
  REQUIRE( f() == nothing );

  weights = {
    { "hello", 0 },
    { "test", 0 },
    { "world", 0 },
  };
  REQUIRE( f() == nothing );
}

TEST_CASE(
    "[irand] pick_from_weighted_values_safe<vector,double>" ) {
  MockIRand impl;
  IRand& irand = impl;

  vector<pair<string, double>> weights;

  auto const f = [&] [[clang::noinline]] {
    return irand.pick_from_weighted_values_safe( weights );
  };

  // NOTE: these will be sorted.
  weights = {
    { "hello", 3.5 },
    { "test", 1 },
    { "world", 5 },
  };
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 0 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 3.499999 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 3.5 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 4.499999 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 4.5 );
  REQUIRE( f() == "world" );
  impl.EXPECT__uniform_double( 0, 9.5 ).returns( 9.499999 );
  REQUIRE( f() == "world" );

  // NOTE: these will be sorted.
  weights = {
    { "hello", 0 },
    { "test", 1.1 },
    { "world", 0 },
  };
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 0 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 1 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_double( 0, 1.1 ).returns( 1.099999 );
  REQUIRE( f() == "test" );

  weights = {};
  REQUIRE( f() == nothing );

  weights = {
    { "hello", 0 },
    { "test", 0 },
    { "world", 0 },
  };
  REQUIRE( f() == nothing );
}

TEST_CASE( "[irand] pick_one_safe" ) {
  MockIRand impl;
  IRand& irand = impl;

  vector<string> values;

  auto const f = [&] [[clang::noinline]] {
    return irand.pick_one_safe( values );
  };

  // NOTE: these will be sorted.
  values = {
    { "hello" },
    { "test" },
    { "world" },
  };
  impl.EXPECT__uniform_int( 0, 2 ).returns( 0 );
  REQUIRE( f() == "hello" );
  impl.EXPECT__uniform_int( 0, 2 ).returns( 1 );
  REQUIRE( f() == "test" );
  impl.EXPECT__uniform_int( 0, 2 ).returns( 2 );
  REQUIRE( f() == "world" );

  values = {};
  REQUIRE( f() == nothing );
}

TEST_CASE( "[irand] bernoulli( Probability )" ) {
  MockIRand impl;
  IRand& irand = impl;

  Probability const p{ .probability = .42 };

  auto const f = [&] [[clang::noinline]] {
    return irand.bernoulli( p );
  };

  impl.EXPECT__bernoulli( .42 ).returns( true );
  REQUIRE( f() == true );
  impl.EXPECT__bernoulli( .42 ).returns( false );
  REQUIRE( f() == false );
}

TEST_CASE( "[irand] NONPORTABLE__sample_piecewise3" ) {
  MockIRand impl;
  IRand& irand = impl;

  Piecewise3Dist p3d;

  auto const f = [&] [[clang::noinline]] {
    return irand.NONPORTABLE__sample_piecewise3( p3d );
  };

  p3d = {
    .min        = .2,
    .peak       = .3,
    .max        = .4,
    .weight_min = .5,
    .weight_max = .7,
  };
  impl.EXPECT__NONPORTABLE__piecewise3( Approx( .5, .0000001 ),
                                        .5, .7 )
      .returns( .9 );
  REQUIRE( f() == .38 );
}

} // namespace
} // namespace rn
