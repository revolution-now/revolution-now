/****************************************************************
**random-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-24.
*
* Description: Unit tests for the rand/random module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rand/random.hpp"

// rand
#include "src/rand/entropy.hpp"
#include "src/rand/real.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

// C++ standard library
#include <atomic>
#include <thread>
#include <vector>

namespace rng {
namespace {

using namespace std;

using ::base::nothing;
using ::Catch::Detail::Approx;

/****************************************************************
** Helpers.
*****************************************************************/
[[nodiscard]] constexpr uint64_t bits_of(
    double const value ) noexcept {
  return bit_cast<uint64_t>( value );
}

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rand/random] construct with seed" ) {
  entropy const e{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };

  int const n = random( e ).uniform<uint32_t>();

  REQUIRE( n == 1911169126 );
}

TEST_CASE( "[rand/random] reseed" ) {
  random r;

  entropy const e{
    .e1 = 0x6151c187,
    .e2 = 0x6da636d6,
    .e3 = 0xfbe4a276,
    .e4 = 0x00f00076,
  };

  r.reseed( e );
  int const n1 = r.uniform_int( 0, 1000 );
  r.reseed( e );
  int const n2 = r.uniform_int( 0, 1000 );
  r.reseed( e );
  int const n3 = r.uniform_int( 0, 1000 );

  REQUIRE( n1 == n2 );
  REQUIRE( n2 == n3 );
}

TEST_CASE( "[rand/random] pick_one_safe" ) {
  random r;

  vector<int> v;

  SECTION( "empty" ) {
    v = {};
    REQUIRE( r.pick_one_safe( v ) == nothing );
    REQUIRE( r.pick_one_safe( v ) == nothing );
  }

  SECTION( "single" ) {
    v = { 111 };
    REQUIRE( r.pick_one_safe( v ) == 111 );
    REQUIRE( r.pick_one_safe( v ) == 111 );
    REQUIRE( r.pick_one_safe( v ) == 111 );
    REQUIRE( r.pick_one_safe( v ) == 111 );
    REQUIRE( r.pick_one_safe( v ) == 111 );
  }

  SECTION( "double" ) {
    v = { 111, 222 };
    REQUIRE( r.pick_one_safe( v ) == 222 );
    REQUIRE( r.pick_one_safe( v ) == 111 );
    REQUIRE( r.pick_one_safe( v ) == 222 );
    REQUIRE( r.pick_one_safe( v ) == 222 );
    REQUIRE( r.pick_one_safe( v ) == 111 );
  }

  SECTION( "triple" ) {
    v = { 111, 222, 333 };
    REQUIRE( r.pick_one_safe( v ) == 333 );
    REQUIRE( r.pick_one_safe( v ) == 111 );
    REQUIRE( r.pick_one_safe( v ) == 333 );
    REQUIRE( r.pick_one_safe( v ) == 333 );
    REQUIRE( r.pick_one_safe( v ) == 111 );
    REQUIRE( r.pick_one_safe( v ) == 333 );
    REQUIRE( r.pick_one_safe( v ) == 333 );
    REQUIRE( r.pick_one_safe( v ) == 111 );
    REQUIRE( r.pick_one_safe( v ) == 222 );
    REQUIRE( r.pick_one_safe( v ) == 111 );
  }
}

TEST_CASE( "[rand/random] pick_one" ) {
  random r;

  vector<int> v;

  SECTION( "single" ) {
    v = { 111 };
    REQUIRE( r.pick_one( v ) == 111 );
    REQUIRE( r.pick_one( v ) == 111 );
    REQUIRE( r.pick_one( v ) == 111 );
    REQUIRE( r.pick_one( v ) == 111 );
    REQUIRE( r.pick_one( v ) == 111 );
  }

  SECTION( "double" ) {
    v = { 111, 222 };
    REQUIRE( r.pick_one( v ) == 222 );
    REQUIRE( r.pick_one( v ) == 111 );
    REQUIRE( r.pick_one( v ) == 222 );
    REQUIRE( r.pick_one( v ) == 222 );
    REQUIRE( r.pick_one( v ) == 111 );
  }

  SECTION( "triple" ) {
    v = { 111, 222, 333 };
    REQUIRE( r.pick_one( v ) == 333 );
    REQUIRE( r.pick_one( v ) == 111 );
    REQUIRE( r.pick_one( v ) == 333 );
    REQUIRE( r.pick_one( v ) == 333 );
    REQUIRE( r.pick_one( v ) == 111 );
    REQUIRE( r.pick_one( v ) == 333 );
    REQUIRE( r.pick_one( v ) == 333 );
    REQUIRE( r.pick_one( v ) == 111 );
    REQUIRE( r.pick_one( v ) == 222 );
    REQUIRE( r.pick_one( v ) == 111 );
  }
}

TEST_CASE( "[rand/random] bernoulli" ) {
  random r;

  // The same probability sequence must produce this exact
  // Boolean sequence on every supported compiler and CPU.
  constexpr array kProbabilities = {
    0.50, 0.25, 0.75, 0.10, 0.90, 0.01, 0.99, 0.50,
    0.25, 0.75, 0.10, 0.90, 0.01, 0.99, 0.50, 0.50,
    0.20, 0.40, 0.60, 0.80, 0.05, 0.95, 0.33, 0.67,
    0.50, 0.25, 0.75, 0.10, 0.90, 0.01, 0.99, 0.50,
  };

  // Populate this once from a trusted build after confirming the
  // unit-value golden vectors. Thereafter, commit the values
  // rather than regenerating them during the test.
  constexpr array kExpected = {
    false, false, true,  false, true,  false, true,  false,
    false, false, false, false, false, true,  false, true,
    false, false, false, false, false, true,  false, false,
    false, false, true,  false, true,  false, true,  true,
  };

  static_assert( kProbabilities.size() == kExpected.size() );

  for( size_t i = 0; i < kProbabilities.size(); ++i ) {
    bool const actual = r.bernoulli( kProbabilities[i] );
    CAPTURE( i );
    CAPTURE( kProbabilities[i] );
    CHECK( actual == kExpected[i] );
  }

  // The implementation deliberately consumes two mt19937 values
  // even when the result is already implied by the endpoint
  // probability.
  for( int i = 0; i < 1000; ++i ) {
    REQUIRE_FALSE( r.bernoulli( 0.0 ) );
    REQUIRE( r.bernoulli( 1.0 ) );
  }
}

TEST_CASE( "[rand/random] uniform_int" ) {
  random r;

  auto constexpr kMin = numeric_limits<int>::min();
  auto constexpr kMax = numeric_limits<int>::max();

  r = {};
  // Sequence of results for fixed inputs.
  REQUIRE( r.uniform_int( 5, 8 ) == 8 );
  REQUIRE( r.uniform_int( 5, 8 ) == 5 );
  REQUIRE( r.uniform_int( 5, 8 ) == 8 );
  REQUIRE( r.uniform_int( 5, 8 ) == 8 );
  REQUIRE( r.uniform_int( 5, 8 ) == 5 );
  REQUIRE( r.uniform_int( 5, 8 ) == 8 );
  REQUIRE( r.uniform_int( 5, 8 ) == 8 );
  REQUIRE( r.uniform_int( 5, 8 ) == 5 );
  REQUIRE( r.uniform_int( 5, 8 ) == 7 );
  REQUIRE( r.uniform_int( 5, 8 ) == 6 );
  REQUIRE( r.uniform_int( 5, 8 ) == 5 );
  REQUIRE( r.uniform_int( 5, 8 ) == 7 );
  REQUIRE( r.uniform_int( 5, 8 ) == 6 );
  REQUIRE( r.uniform_int( 5, 8 ) == 5 );

  r = {};
  // This one exercises the "delta == kMax" branch of the
  // uniform_int function.
  REQUIRE( r.uniform_int( kMin, kMax ) == 1351727964 );
  // This one exercises the "unlikely" branch in the
  // uniform_u32_below function, but not the recursive
  // sub-branch.
  REQUIRE( r.uniform_int( kMin, kMax - 1 ) == -1565614347 );

  r = {};
  // This one exercises the recursive branch of the
  // uniform_u32_below method.
  for( int i = 0; i < 3; ++i ) {
    int a = r.uniform_int( kMin, kMax );
    int b = r.uniform_int( kMin, kMax );
    if( a > b ) swap( a, b );
    (void)r.uniform_int( a, b );
  }
}

TEST_CASE( "[rand/random] uniform_double" ) {
  random r;

  auto const f = [&] [[clang::noinline]] {
    return bits_of( r.uniform_double( 5.5, 8.3 ) );
  };

  REQUIRE( f() == 0x401f1ff9cc07ead7ULL );
  REQUIRE( f() == 0x4020128b16d31378ULL );
  REQUIRE( f() == 0x40176c18b9f7a6bdULL );
  REQUIRE( f() == 0x40201d6a6683a346ULL );
  REQUIRE( f() == 0x401d1519b68d0d2eULL );
  REQUIRE( f() == 0x401717aaf78e20fdULL );
  REQUIRE( f() == 0x40191e8295621c21ULL );
  REQUIRE( f() == 0x401c2004c8ffeb98ULL );
  REQUIRE( f() == 0x40205cae89dffaedULL );
  REQUIRE( f() == 0x40206743a2c49e7dULL );
  REQUIRE( f() == 0x4017c3e880acededULL );
  REQUIRE( f() == 0x40206f711a564b52ULL );
  REQUIRE( f() == 0x40205c31cd523719ULL );
  REQUIRE( f() == 0x401b6fab477c2388ULL );
  REQUIRE( f() == 0x401ef6906da353a5ULL );
  REQUIRE( f() == 0x401796d10806741eULL );
  REQUIRE( f() == 0x401ab94622260f42ULL );
  REQUIRE( f() == 0x402020cc67052ec4ULL );
  REQUIRE( f() == 0x401edf6ab63b703dULL );
  REQUIRE( f() == 0x40205f87415a0541ULL );
}

TEST_CASE( "[rand/random] unit" ) {
  random r;

  auto const f = [&] [[clang::noinline]] {
    return bits_of( r.unit() );
  };

  REQUIRE( f() == 0x3fea12376b8455d3ULL );
  REQUIRE( f() == 0x3fecfc3f5ddab863ULL );
  REQUIRE( f() == 0x3fc0411a967c03d8ULL );
  REQUIRE( f() == 0x3fed3a6000a712b5ULL );
  REQUIRE( f() == 0x3fe43c497749dc81ULL );
  REQUIRE( f() == 0x3fb8f868638c16a0ULL );
  REQUIRE( f() == 0x3fd1d2ea310c0e72ULL );
  REQUIRE( f() == 0x3fe1800dabffc5afULL );
  REQUIRE( f() == 0x3feea3e55cffe302ULL );
  REQUIRE( f() == 0x3feee05deb3ef75bULL );
  REQUIRE( f() == 0x3fc42caa50dce848ULL );
  REQUIRE( f() == 0x3fef0f18967f6540ULL );
  REQUIRE( f() == 0x3feea11c954383fcULL );
  REQUIRE( f() == 0x3fdf10650633142aULL );
  REQUIRE( f() == 0x3fe99be5cb898143ULL );
  REQUIRE( f() == 0x3fc22954ee009cbcULL );
  REQUIRE( f() == 0x3fdafe230c47329eULL );
  REQUIRE( f() == 0x3fed4db495d478f1ULL );
  REQUIRE( f() == 0x3fe959c32d3c1c1aULL );
  REQUIRE( f() == 0x3feeb42975701e01ULL );
}

TEST_CASE( "[rand/random] NONPORTABLE__normal" ) {
  random r;
  double const d = r.NONPORTABLE__normal( 1.5, 2.3 );
#ifdef _LIBCPP_VERSION
  REQUIRE( d == Approx( 1.1633219033 ) );
#else
  REQUIRE( d == Approx( 1.8094182145 ) );
#endif
}

TEST_CASE( "[rand/random] NONPORTABLE__piecewise (3)" ) {
  random r;
  random::piecewise3 const p3{
    .l = { .value = 2.5, .weight = .5 },
    .m = { .value = 3.0, .weight = 1.0 },
    .r = { .value = 4.2, .weight = 0.2 } };
  double const d = r.NONPORTABLE__piecewise( p3 );
  REQUIRE( d == Approx( 2.7393880168 ) );
}

// Deterministic since there is no seeding and we are only using
// the uniform_int which we implement ourselves.
TEST_CASE( "[rand/random] uniform_int full range coverage" ) {
  SECTION( "small int range" ) {
    random r; // NOTE: no seeding.
    static int constexpr kMin   = 3;
    static int constexpr kMax   = 17;
    static int constexpr kCount = kMax - kMin + 1;
    vector<uint8_t> s( kCount );
    int64_t count  = 0;
    uint32_t found = 0;
    while( found < kCount ) {
      int64_t x = r.uniform_int( kMin, kMax );
      CHECK( x >= kMin );
      CHECK( x <= kMax );
      x -= kMin;
      auto& r = s.at( x );
      if( !r ) {
        r = 1;
        ++found;
      }
      ++count;
    }
    REQUIRE( count == 44 );
    REQUIRE( found == 15 );
  }

  // This runs a cpu-intensive test with multiple threads to en-
  // sure that the uniform_int generator, on the full range of
  // ints, eventually produces all ~4.3 billion values. Since the
  // implementation of uniform_int, in that scenario, defers to
  // raw(), it is effectively testing that as well.
  //
  // NOTE: This takes ~6 mins with 16 cores, thus it is disabled
  //       by default.  Although we have a helper method called
  //       testing::expensive_tests_enabled(), this test is even
  //       too expensive for that, so it just stays off.
  SECTION( "full int range" ) {
#if 0
    vector<atomic<uint8_t>> s( uint64_t( 0xffffffff ) + 1 );
    static auto constexpr kMin = numeric_limits<int>::min();
    static auto constexpr kMax = numeric_limits<int>::max();
    static auto constexpr kAdd =
        -int64_t( numeric_limits<int>::min() );
    int const num_threads = thread::hardware_concurrency();
    vector<atomic<bool>> stops( num_threads );
    auto const f = [&]( int const idx ) {
      auto& stop = stops[idx];
      // Here we need a separate generator for each thread be-
      // cause they are not thread safe. Also, we need a separate
      // seed in each thread otherwise all threads would just
      // find the exact same values and there would be no speedup
      // from multiple threads.
      random r;
      r.reseed( entropy::from_random_device() );
      while( !stop ) {
        int64_t x = r.uniform_int( kMin, kMax );
        x += kAdd;
        s[x] = 1;
      }
    };
    auto const printer = [&] {
      while( true ) {
        int64_t found = 0;
        for( auto const& e : s )
          if( e.load() ) //
            ++found;
        if( found > 0 )
          fmt::println( "found={:#010X}",
                        uint32_t( found - 1 ) );
        if( found == uint64_t( 0xffffffff ) + 1 ) break;
      }
      for( auto& stop : stops ) stop = true;
    };
    vector<jthread> threads;
    for( int i = 0; i < num_threads; ++i )
      threads.emplace_back( f, i );
    threads.emplace_back( printer );
    for( jthread& jt : threads ) jt.join();
#endif
  }
}

TEST_CASE( "[rand/random] uniform<T>" ) {
  random r;

  SECTION( "uint8_t" ) {
    REQUIRE( r.uniform<uint8_t>() == 92 );
    REQUIRE( r.uniform<uint8_t>() == 246 );
    REQUIRE( r.uniform<uint8_t>() == 238 );
    REQUIRE( r.uniform<uint8_t>() == 121 );
    REQUIRE( r.uniform<uint8_t>() == 44 );
    REQUIRE( r.uniform<uint8_t>() == 223 );
    REQUIRE( r.uniform<uint8_t>() == 5 );
    REQUIRE( r.uniform<uint8_t>() == 225 );
  }

  SECTION( "unsigned short" ) {
    REQUIRE( r.uniform<unsigned short>() == 47964 );
    REQUIRE( r.uniform<unsigned short>() == 40694 );
    REQUIRE( r.uniform<unsigned short>() == 64238 );
    REQUIRE( r.uniform<unsigned short>() == 8057 );
    REQUIRE( r.uniform<unsigned short>() == 13612 );
    REQUIRE( r.uniform<unsigned short>() == 47071 );
    REQUIRE( r.uniform<unsigned short>() == 5 );
    REQUIRE( r.uniform<unsigned short>() == 45025 );
  }
}

// It was once said that a default-constructed std::mt19937 must
// produce the value 4123659995 as its 10,000th output on every
// compliant implementation.
TEST_CASE( "[rand/random] marsenne determinacy" ) {
  // This may not be guaranteed by the compiler but we want to
  // know if it is violated, since it probably shouldn't be on
  // the relevant platforms.
  static_assert( is_same_v<random::result_type, uint64_t> );

  random::result_type constexpr mt19937_ten_thousandth_output =
      4123659995;

  SECTION( "marsenne directly" ) {
    random::engine_t engine;
    for( int i = 0; i < 10'000 - 1; ++i ) engine();
    REQUIRE( engine() == mt19937_ten_thousandth_output );
  }

  SECTION( "random object" ) {
    random r;
    for( int i = 0; i < 10'000 - 1; ++i ) (void)r.raw();
    REQUIRE( r.raw() == mt19937_ten_thousandth_output );
  }
}

TEST_CASE( "[rand/random] new_deterministic_seed" ) {
  random r;

  auto const f = [&] [[clang::noinline]] {
    return r.new_deterministic_seed();
  };

  // Should be deterministic because rand by default will not
  // seed the underlying engine and the implementation of the
  // uniform methods used don't include any further platform de-
  // pendent stuff. Thus our marsenne twister engine should give
  // consistent results, even across platforms it is said.

  REQUIRE( base::to_str( f() ) ==
           "sd5c31f79e7e1faee22ae9ef6d091bb5c" );

  REQUIRE( base::to_str( f() ) ==
           "s3895afe1e9d30005f807b7df2082352c" );
}

} // namespace
} // namespace rng
