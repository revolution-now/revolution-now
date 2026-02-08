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

// TODO: this is technically not deterministic until we implement
// our own bernoulli distribution.
TEST_CASE( "[rand/random] bernoulli" ) {
  random r;
  REQUIRE( r.bernoulli( .7 ) == true );
  REQUIRE( r.bernoulli( .7 ) == false );
  REQUIRE( r.bernoulli( .7 ) == false );
  REQUIRE( r.bernoulli( .7 ) == true );
}

TEST_CASE( "[rand/random] uniform_int" ) {
  random r;
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
}

TEST_CASE( "[rand/random] uniform_double" ) {
  random r;
  double const d = r.uniform_double( 5.5, 8.3 );
  REQUIRE( ( d >= 5.5 && d <= 8.3 ) );
}

TEST_CASE( "[rand/random] normal" ) {
  random r;
  double const d = r.normal( 1.5, 2.3 );
  REQUIRE( d == 1.8094182145 );
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
  //       by default.
  //
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
        this_thread::sleep_for( chrono::milliseconds{ 10000 } );
        int64_t found = 0;
        for( auto const& e : s )
          if( e.load() ) //
            ++found;
        fmt::println( "found={:L}", found );
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

TEST_CASE( "[rand/random] generate_deterministic_seed" ) {
  random r;

  auto const f = [&] [[clang::noinline]] {
    return r.generate_deterministic_seed();
  };

  // Should be deterministic because rand by default will not
  // seed the underlying engine and the implementation of the
  // uniform methods used don't include any further platform de-
  // pendent stuff. Thus our marsenne twister engine should give
  // consistent results, even across platforms it is said.

  REQUIRE( base::to_str( f() ) ==
           "d5c31f79e7e1faee22ae9ef6d091bb5c" );

  REQUIRE( base::to_str( f() ) ==
           "3895afe1e9d30005f807b7df2082352c" );
}

} // namespace
} // namespace rng
