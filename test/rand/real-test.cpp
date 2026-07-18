/****************************************************************
**real-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-07-14.
*
* Description: Unit tests for the rand/real module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rand/real.hpp"

// C++ standard library
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rng {
namespace {

using namespace std;

/****************************************************************
** Helpers.
*****************************************************************/
[[nodiscard]] constexpr uint64_t bits_of(
    double const value ) noexcept {
  return bit_cast<uint64_t>( value );
}

/****************************************************************
** Golden data.
*****************************************************************/
// The seed used for all golden-vector tests.
//
// Using a fixed-width integer makes the intended seed value ex-
// plicit.
uint32_t constexpr kSeed = 0x12345678u;

// These bounds are exactly representable binary floating-point
// values:
//
//   min = -40.0
//   max = 100.0
//
// Hexadecimal floating-point literals make their exact values
// explicit.
double constexpr kMin = -0x1.4p+5;
double constexpr kMax = 0x1.9p+6;

static_assert( kMin == -40.0 );
static_assert( kMax == 100.0 );

array<uint64_t, 32> constexpr kExpectedUnitResults = {
  0x3fe8d2f268612c5aULL, 0x3fe4e744949c2314ULL,
  0x3fe6a8eb576c8c27ULL, 0x3fdbccbd36fc9efcULL,
  0x3fd193763725707eULL, 0x3fe6dbebff105720ULL,
  0x3fa8a92887ce30f0ULL, 0x3fad409a1f14eff0ULL,
  0x3fcce5d8cd4eda14ULL, 0x3feaad6ea1d64540ULL,
  0x3fa7d5c22b19f080ULL, 0x3fa92b434099bb30ULL,
  0x3fcdac26f9dfd0d4ULL, 0x3fd8d2d81cfa944cULL,
  0x3fd58500ebbfa3c8ULL, 0x3fd5d400479b26b0ULL,
  0x3fd23a0c09c9ef04ULL, 0x3fb9132cdf3a0cb0ULL,
  0x3fec12a10cfa0f12ULL, 0x3f7f440016e74300ULL,
  0x3fefc497ac24ade1ULL, 0x3fbaf73f23430e50ULL,
  0x3fa7bad9a09a66f0ULL, 0x3fe655fed58761a1ULL,
  0x3fe0a4eb9436eec9ULL, 0x3fe86e335b50749aULL,
  0x3f9cf0c45247cc40ULL, 0x3fe349312a7f2b76ULL,
  0x3fba4097331b43c0ULL, 0x3fdaa37e3aa14898ULL,
  0x3fe674d23d0d384dULL, 0x3fbc27ac47cd1fb0ULL,
};

array<uint64_t, 32> constexpr kExpectedRangedResults = {
  0x405126b9222a4882ULL, 0x4049b9e605158cbcULL,
  0x404d9182cf3d7294ULL, 0x4034cfdde8489bc8ULL,
  0xbff8d6d675e19ec0ULL, 0x404e01143df3be96ULL,
  0xc040a0df756ecf50ULL, 0xc040002aedc02332ULL,
  0xc020c935bee382f4ULL, 0x40532db101025bbeULL,
  0xc040bdc6741b741eULL, 0xc0408f15ce2afb68ULL,
  0xc01e2ed57accce60ULL, 0x402c9a717ec848ccULL,
  0x401c4bc80eccd914ULL, 0x401eff02728d9288ULL,
  0xbfc082d54b229400ULL, 0xc03a498375ec410fULL,
  0x4054b4602631807cULL, 0xc04377367f9bcc3bULL,
  0x4058bf05e4481e2eULL, 0xc03940c978b7542dULL,
  0xc040c1743f0ae3eeULL, 0x404cdc1d73182590ULL,
  0x404068c354382a58ULL, 0x4050b8882bdfff88ULL,
  0xc042058a9460178aULL, 0x4046301b8cf62f12ULL,
  0xc039a4ad500d16f3ULL, 0x403245a42040cecdULL,
  0x404d1f8be58ceb28ULL, 0xc0389a4dc8bbd2abULL,
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[rand/real] floating-point environment satisfies "
    "portability requirements" ) {
  // This function should contain all runtime checks, including:
  //
  //   assert( std::fegetround() == FE_TONEAREST );
  //
  // and any architecture-specific FTZ/DAZ checks your supported
  // platforms require.
  check_floating_point_portability();

  SUCCEED(
      "All runtime floating-point portability checks passed." );
}

TEST_CASE(
    "[rand/real] portable distribution constants have expected "
    "bit patterns" ) {
  // Verify the assumptions underlying bit_cast<double,
  // uint64_t>.
  REQUIRE( sizeof( double ) == sizeof( uint64_t ) );
  REQUIRE( numeric_limits<double>::is_iec559 );
  REQUIRE( numeric_limits<double>::radix == 2 );
  REQUIRE( numeric_limits<double>::digits == 53 );
  REQUIRE( numeric_limits<double>::max_exponent == 1024 );
  REQUIRE( numeric_limits<double>::min_exponent == -1021 );

  // Canonical binary64 representations.
  REQUIRE( bits_of( 0.0 ) == 0x0000000000000000ULL );
  REQUIRE( bits_of( -0.0 ) == 0x8000000000000000ULL );
  REQUIRE( bits_of( 1.0 ) == 0x3ff0000000000000ULL );
  REQUIRE( bits_of( -1.0 ) == 0xbff0000000000000ULL );
  REQUIRE( bits_of( 0.5 ) == 0x3fe0000000000000ULL );
  REQUIRE( bits_of( 2.0 ) == 0x4000000000000000ULL );

  // Constants used by this test suite.
  REQUIRE( bits_of( kMin ) == 0xc044000000000000ULL );
  REQUIRE( bits_of( kMax ) == 0x4059000000000000ULL );

  // Constants used by the unit-interval conversion.
  REQUIRE( bits_of( 0x1p-53 ) == 0x3ca0000000000000ULL );
  REQUIRE( bits_of( 1.0 - 0x1p-53 ) == 0x3fefffffffffffffULL );

  // Important representational boundaries.
  REQUIRE( bits_of( numeric_limits<double>::min() ) ==
           0x0010000000000000ULL );
  REQUIRE( bits_of( numeric_limits<double>::denorm_min() ) ==
           0x0000000000000001ULL );
  REQUIRE( bits_of( numeric_limits<double>::max() ) ==
           0x7fefffffffffffffULL );
  REQUIRE( bits_of( numeric_limits<double>::infinity() ) ==
           0x7ff0000000000000ULL );
  REQUIRE( bits_of( -numeric_limits<double>::infinity() ) ==
           0xfff0000000000000ULL );
}

TEST_CASE(
    "[rand/real] std::mt19937 has the required standardized "
    "properties" ) {
  REQUIRE( mt19937::min() == 0u );
  REQUIRE( mt19937::max() == 0xffffffffu );

  REQUIRE( numeric_limits<mt19937::result_type>::digits >= 32 );

  REQUIRE( numeric_limits<uint32_t>::digits == 32 );
  REQUIRE( numeric_limits<uint64_t>::digits == 64 );
}

TEST_CASE(
    "[rand/real] portable_uniform_real_distribution produces "
    "expected unit-interval bits" ) {
  portable_uniform_real_distribution distribution;
  mt19937 generator( kSeed );

  for( size_t i = 0; i < kExpectedUnitResults.size(); ++i ) {
    double const value = distribution( generator );

    CAPTURE( i );
    CAPTURE( value );
    CAPTURE( bits_of( value ) );

    REQUIRE( value >= 0.0 );
    REQUIRE( value < 1.0 );

    // Every output must lie on the exact 2^-53 grid.
    //
    // Multiplication by 2^53 must therefore yield an exact
    // integer.
    double const scaled = value * 0x1p+53;

    REQUIRE( scaled >= 0.0 );
    REQUIRE( scaled < 0x1p+53 );
    REQUIRE( scaled == static_cast<uint64_t>( scaled ) );

    REQUIRE( bits_of( value ) == kExpectedUnitResults[i] );
  }
}

TEST_CASE(
    "[rand/real] portable_uniform_real_distribution produces "
    "expected ranged bits" ) {
  portable_uniform_real_distribution distribution;
  mt19937 generator( kSeed );

  for( size_t i = 0; i < kExpectedRangedResults.size(); ++i ) {
    double const value = distribution( generator, kMin, kMax );

    CAPTURE( i );
    CAPTURE( value );
    CAPTURE( bits_of( value ) );

    REQUIRE( value >= kMin );
    REQUIRE( value < kMax );

    REQUIRE( bits_of( value ) == kExpectedRangedResults[i] );
  }
}

TEST_CASE(
    "[rand/real] portable distribution consumes exactly two "
    "engine values" ) {
  portable_uniform_real_distribution distribution;

  mt19937 distribution_generator( kSeed );
  mt19937 reference_generator( kSeed );

  size_t constexpr kSampleCount = 10'000;

  for( size_t i = 0; i < kSampleCount; ++i ) {
    static_cast<void>( distribution( distribution_generator ) );

    static_cast<void>( reference_generator() );
    static_cast<void>( reference_generator() );

    CAPTURE( i );

    REQUIRE( distribution_generator == reference_generator );
  }
}

TEST_CASE(
    "[rand/real] unit-interval generation is exactly "
    "reproducible" ) {
  portable_uniform_real_distribution distribution;

  mt19937 first( kSeed );
  mt19937 second( kSeed );

  size_t constexpr kSampleCount = 100'000;

  for( size_t i = 0; i < kSampleCount; ++i ) {
    double const first_value  = distribution( first );
    double const second_value = distribution( second );

    CAPTURE( i );
    REQUIRE( bits_of( first_value ) == bits_of( second_value ) );
  }

  REQUIRE( first == second );
}

TEST_CASE(
    "[rand/real] ranged generation is exactly reproducible" ) {
  portable_uniform_real_distribution distribution;

  mt19937 first( kSeed );
  mt19937 second( kSeed );

  size_t constexpr kSampleCount = 100'000;

  for( size_t i = 0; i < kSampleCount; ++i ) {
    double const first_value = distribution( first, kMin, kMax );

    double const second_value =
        distribution( second, kMin, kMax );

    CAPTURE( i );

    REQUIRE( first_value >= kMin );
    REQUIRE( first_value < kMax );

    REQUIRE( bits_of( first_value ) == bits_of( second_value ) );
  }

  REQUIRE( first == second );
}

TEST_CASE(
    "[rand/real] portable_uniform_real_distribution respects "
    "many finite ranges" ) {
  struct Range {
    double min = {};
    double max = {};
  };

  // Every endpoint is written as an exact hexadecimal
  // floating-point literal so its bit pattern is unambiguous.
  array constexpr ranges = {
    Range{ 0x0p+0, 0x1p+0 },
    Range{ -0x1p+0, 0x1p+0 },
    Range{ -0x1.4p+5, 0x1.9p+6 },
    Range{ 0x1p-100, 0x1p-90 },
    Range{ -0x1p-90, -0x1p-100 },
    Range{ 0x1p+500, 0x1.8p+500 },
    Range{ -0x1.8p+500, -0x1p+500 },
    Range{ -0x1p+500, 0x1p+500 },
    Range{ -0x1p-500, 0x1p-500 },
    Range{ 0x1.0000000000000p+0, 0x1.0000000000100p+0 },
  };

  portable_uniform_real_distribution distribution;

  size_t constexpr kSamplesPerRange = 1'000;

  for( size_t range_index = 0; range_index < ranges.size();
       ++range_index ) {
    Range const range = ranges[range_index];

    REQUIRE( range.min < range.max );

    mt19937 generator( kSeed +
                       static_cast<uint32_t>( range_index ) );

    for( size_t i = 0; i < kSamplesPerRange; ++i ) {
      double const value =
          distribution( generator, range.min, range.max );

      CAPTURE( range_index );
      CAPTURE( i );
      CAPTURE( range.min );
      CAPTURE( range.max );
      CAPTURE( value );
      CAPTURE( bits_of( value ) );

      REQUIRE( value >= range.min );
      REQUIRE( value < range.max );
    }
  }
}

TEST_CASE(
    "[rand/real] unit-interval distribution reaches both "
    "halves" ) {
  portable_uniform_real_distribution distribution;
  mt19937 generator( kSeed );

  bool saw_lower_half = false;
  bool saw_upper_half = false;
  bool saw_nonzero    = false;

  size_t constexpr kSampleCount = 100'000;

  for( size_t i = 0; i < kSampleCount; ++i ) {
    double const value = distribution( generator );

    REQUIRE( value >= 0.0 );
    REQUIRE( value < 1.0 );

    saw_lower_half |= value < 0.5;
    saw_upper_half |= value >= 0.5;
    saw_nonzero |= value != 0.0;
  }

  REQUIRE( saw_lower_half );
  REQUIRE( saw_upper_half );
  REQUIRE( saw_nonzero );
}

TEST_CASE(
    "[rand/real] portable_uniform_real_distribution is "
    "statistically uniform" ) {
  if( !testing::expensive_tests_enabled() ) return;
  uint32_t constexpr kSeed = 0x12345678u;
  // NOTE: there are bounds further below that depend on the
  // sample size, so changing this might break stuff.
  uint64_t constexpr kSampleCount = 10'000'000;
  size_t constexpr kBucketCount   = 1'024;

  portable_uniform_real_distribution distribution;
  mt19937 generator( kSeed );

  array<uint64_t, kBucketCount> buckets{};

  // CDF = cumulative distribution function.

  // Empirical-CDF checkpoints at 0.1, 0.2, ..., 0.9.
  array<double, 9> constexpr kCdfPoints = {
    0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9,
  };

  array<uint64_t, kCdfPoints.size()> cdf_counts{};

  // Long double reduces accumulation error. The tolerances below
  // are much larger than any plausible accumulation differences
  // between platforms.
  long double sum         = 0.0L;
  long double sum_squares = 0.0L;

  long double adjacent_product_sum = 0.0L;
  double previous                  = 0.0;

  double minimum = 1.0;
  double maximum = 0.0;

  for( uint64_t i = 0; i < kSampleCount; ++i ) {
    double const value = distribution( generator );

    REQUIRE( value >= 0.0 );
    REQUIRE( value < 1.0 );

    minimum = min( minimum, value );
    maximum = max( maximum, value );

    long double const wide_value =
        static_cast<long double>( value );

    sum += wide_value;
    sum_squares += wide_value * wide_value;

    if( i != 0 )
      adjacent_product_sum +=
          static_cast<long double>( previous ) * wide_value;

    previous = value;

    // value is strictly less than 1.0, so the converted bucket
    // index must be in [0, bucket_count).
    size_t const bucket = static_cast<size_t>(
        value * static_cast<double>( kBucketCount ) );

    BASE_CHECK( bucket < kBucketCount );
    ++buckets[bucket];

    for( size_t checkpoint = 0; checkpoint < kCdfPoints.size();
         ++checkpoint ) {
      if( value < kCdfPoints[checkpoint] )
        ++cdf_counts[checkpoint];
    }
  }

  long double const n = static_cast<long double>( kSampleCount );

  long double const mean = sum / n;

  long double const second_moment = sum_squares / n;

  long double const variance = second_moment - mean * mean;

  INFO( "sample count: " << kSampleCount );
  INFO( "minimum: " << minimum );
  INFO( "maximum: " << maximum );
  INFO( "mean: " << static_cast<double>( mean ) );
  INFO( "variance: " << static_cast<double>( variance ) );

  SECTION( "sample mean is consistent with U[0,1)" ) {
    long double constexpr expected_mean = 0.5L;

    // For U[0,1), standard error of the mean is:
    //
    //   sqrt((1/12) / N)
    //
    // At ten million samples this is about 9.13e-5. This toler-
    // ance is roughly eight standard errors.
    long double constexpr tolerance = 0.000'75L;

    REQUIRE( abs( mean - expected_mean ) < tolerance );
  }

  SECTION( "sample variance is consistent with U[0,1)" ) {
    long double constexpr expected_variance = 1.0L / 12.0L;

    // At ten million samples, the standard error of the
    // population-form variance estimate is approximately
    // 2.36e-5. This is roughly an eight-standard-error toler-
    // ance.
    long double constexpr tolerance = 0.000'20L;

    REQUIRE( abs( variance - expected_variance ) < tolerance );
  }

  SECTION( "histogram passes a chi-square test" ) {
    long double constexpr expected_per_bucket =
        static_cast<long double>( kSampleCount ) /
        static_cast<long double>( kBucketCount );

    long double chi_square = 0.0L;

    uint64_t smallest_bucket_count = kSampleCount;
    uint64_t largest_bucket_count  = 0;

    for( size_t bucket = 0; bucket < kBucketCount; ++bucket ) {
      uint64_t const observed = buckets[bucket];

      smallest_bucket_count =
          min( smallest_bucket_count, observed );

      largest_bucket_count =
          max( largest_bucket_count, observed );

      long double const difference =
          static_cast<long double>( observed ) -
          expected_per_bucket;

      chi_square +=
          difference * difference / expected_per_bucket;
    }

    long double constexpr degrees_of_freedom =
        static_cast<long double>( kBucketCount - 1 );

    // A chi-square variable with k degrees of freedom has:
    //
    //   mean = k
    //   standard deviation = sqrt(2k)
    //
    // We use a deliberately generous eight-standard-deviation
    // interval.
    long double const standard_deviation =
        sqrt( 2.0L * degrees_of_freedom );

    long double const lower_limit =
        degrees_of_freedom - 8.0L * standard_deviation;

    long double const upper_limit =
        degrees_of_freedom + 8.0L * standard_deviation;

    INFO( "chi square: " << static_cast<double>( chi_square ) );

    INFO( "degrees of freedom: "
          << static_cast<double>( degrees_of_freedom ) );

    INFO( "allowed interval: ["
          << static_cast<double>( lower_limit ) << ", "
          << static_cast<double>( upper_limit ) << "]" );

    INFO( "smallest bucket count: " << smallest_bucket_count );

    INFO( "largest bucket count: " << largest_bucket_count );

    REQUIRE( chi_square > lower_limit );
    REQUIRE( chi_square < upper_limit );
  }

  SECTION( "empirical CDF matches the uniform CDF" ) {
    // For each fixed threshold p, the count below p is binomial
    // with:
    //
    //   mean = Np
    //   standard deviation = sqrt(Np(1-p))
    //
    // Use an eight-standard-deviation bound at every checkpoint.
    for( size_t i = 0; i < kCdfPoints.size(); ++i ) {
      long double const p =
          static_cast<long double>( kCdfPoints[i] );

      long double const expected_count = n * p;

      long double const standard_deviation =
          sqrt( n * p * ( 1.0L - p ) );

      long double const difference =
          abs( static_cast<long double>( cdf_counts[i] ) -
               expected_count );

      CAPTURE( i );
      CAPTURE( kCdfPoints[i] );
      CAPTURE( cdf_counts[i] );
      CAPTURE( static_cast<double>( expected_count ) );
      CAPTURE( static_cast<double>( standard_deviation ) );
      CAPTURE( static_cast<double>( difference ) );

      REQUIRE( difference < 8.0L * standard_deviation );
    }
  }

  SECTION( "successive values have negligible correlation" ) {
    long double const pair_count =
        static_cast<long double>( kSampleCount - 1 );

    long double const adjacent_product_mean =
        adjacent_product_sum / pair_count;

    // For a uniform variable:
    //
    //   E[X]   = 1/2
    //   Var(X) = 1/12
    //
    // Covariance at lag one is estimated as E[X_i X_(i+1)] -
    // E[X]^2.
    long double const covariance =
        adjacent_product_mean - mean * mean;

    long double const correlation = covariance / variance;

    INFO( "lag-one covariance: "
          << static_cast<double>( covariance ) );

    INFO( "lag-one correlation: "
          << static_cast<double>( correlation ) );

    // For independent samples, the approximate standard error of
    // a sample correlation is 1/sqrt(N), about 0.000316 here.
    // This tolerance is comfortably greater than eight standard
    // errors.
    REQUIRE( abs( correlation ) < 0.003L );
  }

  SECTION( "samples cover the interval broadly" ) {
    // These are coarse sanity checks. With ten million samples,
    // failure would indicate a severe distribution defect.
    REQUIRE( minimum < 0.000'01 );
    REQUIRE( maximum > 0.999'99 );

    REQUIRE( buckets.front() != 0 );
    REQUIRE( buckets.back() != 0 );
  }
}

} // namespace
} // namespace rng
