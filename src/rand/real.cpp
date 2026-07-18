/****************************************************************
**real.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-07-13.
*
* Description: A platform-independent uniform double generator.
*
*****************************************************************/
#include "real.hpp"

// base
#include "base/error.hpp"

// C++ standard library
#include <cfenv>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

#if defined( _M_IX86 ) || defined( _M_X64 )
#  include <xmmintrin.h>
#endif

namespace rng {

namespace {

using namespace std;

void check_portable_rounding_mode() {
  CHECK( fegetround() == FE_TONEAREST );
}

void check_subnormal_modes() {
#if defined( _M_IX86 ) || defined( _M_X64 )
  unsigned int const mxcsr = _mm_getcsr();

  constexpr unsigned int daz = 1u << 6;
  constexpr unsigned int ftz = 1u << 15;

  CHECK( ( mxcsr & daz ) == 0,
         "denormals-are-zero mode must be disabled." );

  CHECK( ( mxcsr & ftz ) == 0,
         "flush-to-zero mode must be disabled." );
#endif
}

} // namespace

/****************************************************************
** Control Floating Point Environment.
*****************************************************************/
// In this section are the static conditions pertaining to the
// floating point environment and compilation mode that are
// likely required to ensure reproducible random double numbers
// (both the [0,1.0) base method and the range method [a,b).

// Require IEEE-754-style floating-point behavior.
//
// This excludes unusual floating-point implementations where in-
// finities, NaNs, rounding, and representable values do not
// follow IEC 60559 rules.
static_assert( std::numeric_limits<double>::is_iec559,
               "double must conform to IEC 60559 / IEEE 754." );

// Require a binary floating-point representation.
//
// The conversion algorithm constructs a binary fraction by mul-
// tiplying an integer by exactly 2^-53. That reasoning does not
// apply to decimal or other non-binary floating-point formats.
static_assert( std::numeric_limits<double>::radix == 2,
               "double must use a binary representation." );

// Require exactly the usual binary64 precision.
//
// Every integer in [0, 2^53] is then exactly representable as a
// double, and multiplying by 2^-53 produces the intended 53-bit
// binary fraction.
//
// Using == 53, rather than >= 53, also ensures that the ranged
// arithmetic uses the same precision on every supported plat-
// form.
static_assert(
    std::numeric_limits<double>::digits == 53,
    "double must have exactly 53 bits of binary precision." );

// Require the standard binary64 exponent range.
//
// The preceding checks establish IEEE binary floating point with
// 53 bits of precision, but IEEE 754 also defines formats other
// than binary64. These checks ensure that double is specifically
// the usual binary64 format.
static_assert(
    std::numeric_limits<double>::max_exponent == 1024,
    "double must have the binary64 maximum exponent." );

static_assert(
    std::numeric_limits<double>::min_exponent == -1021,
    "double must have the binary64 minimum exponent." );

// Require floating-point expressions to be evaluated at their
// declared precision.
//
// Without this, an implementation may evaluate double expres-
// sions using wider intermediates, such as the 80-bit x87 for-
// mat. That can cause the ranged interpolation to round differ-
// ently from platforms that evaluate every operation directly as
// binary64.
static_assert(
    FLT_EVAL_METHOD == 0,
    "double expressions must be evaluated as double." );

// Require support for subnormal double values.
//
// If interpolation produces a subnormal result, platforms that
// do not support subnormals cannot reproduce the same IEEE bina-
// ry64 result.
static_assert( std::numeric_limits<double>::has_denorm ==
                   std::denorm_present,
               "double must support subnormal values." );

// Require uint32_t to exist as an exact 32-bit unsigned integer
// type.
//
// std::mt19937 produces 32-bit values. The implementation stores
// each draw in uint32_t and relies on those values containing
// exactly 32 bits.
static_assert( std::numeric_limits<std::uint32_t>::digits == 32,
               "uint32_t must contain exactly 32 value bits." );

// Require uint64_t to exist as an exact 64-bit unsigned integer
// type.
//
// Two mt19937 outputs are concatenated into one 64-bit integer
// before the upper 53 bits are extracted.
static_assert( std::numeric_limits<std::uint64_t>::digits == 64,
               "uint64_t must contain exactly 64 value bits." );

// Verify that std::mt19937 has the standardized zero lower
// bound.
//
// This implementation treats each engine output directly as a
// 32-bit word and therefore does not subtract the engine's min-
// imum value.
static_assert(
    std::mt19937::min() == 0,
    "std::mt19937 must have a minimum result of zero." );

// Verify that std::mt19937 spans the complete 32-bit unsigned
// range.
//
// The concatenation logic assumes every call supplies 32 uni-
// formly distributed bits, with all values from 0 through 2^32-1
// possible.
static_assert( std::mt19937::max() ==
                   std::numeric_limits<std::uint32_t>::max(),
               "std::mt19937 must produce the full 32-bit "
               "unsigned range." );

// There are additional conditions that must be enforced through
// compiler flags or runtime configuration:
//
// 1. Floating-point contraction must be disabled.
//    Example GCC/Clang flag:
//      -ffp-contract=off
//
// 2. Fast-math and reassociation must be disabled.
//    Avoid:
//      -ffast-math
//      -Ofast
//
// 3. The runtime rounding mode must remain:
//      round-to-nearest, ties-to-even
//
// 4. Flush-to-zero and denormals-are-zero modes must remain dis-
//    abled.
//
// 5. All platforms must receive identical bit patterns for min
//    and max. Decimal source literals can be relied upon only
//    when correctly rounded by the compiler; serialized values
//    should use a fixed binary encoding when strict repro-
//    ducibility matters.

// Here are some additional things to consider for floating point
// reproducibility:
//
//   * The code could be inlined or instantiated in a translation
//     unit compiled with different flags. Neither a
//     static_assert nor a runtime assertion can prove that all
//     participating translation units used the same
//     floating-point options.
//   * Thus, keep the implementation in a source file (i.e., this
//     one) compiled with controlled flags, or ensure the build
//     system applies the flags target-wide.
//   * Link-time optimization must also receive compatible
//     floating-point settings.

/****************************************************************
** portable_uniform_real_distribution<std::mt19937>
*****************************************************************/
double portable_uniform_real_distribution::operator()(
    mt19937& gen ) const {
  // Separate statements guarantee the order in which the
  // engine is advanced.
  uint32_t const hi = gen();
  uint32_t const lo = gen();

  uint64_t const bits =
      ( uint64_t{ hi } << 32 ) | uint64_t{ lo };

  // Keep the upper 53 bits and divide exactly by 2^53.
  return static_cast<double>( bits >> 11 ) * 0x1p-53;
}

double portable_uniform_real_distribution::operator()(
    mt19937& gen, double const min, double const max ) const {
  // This one is needed to ensure we have the correct rounding
  // mode set which is necessary for reproducible floating point
  // math which we need to ensure consistent rng and rounding be-
  // havior. This can change at runtime theoretically so the
  // safest thing to do is to check it regularly. The engine will
  // also check this independently when initializing the rng
  // system in order to support a fast fail if it is violated.
  check_portable_rounding_mode();

  CHECK( isfinite( min ) );
  CHECK( isfinite( max ) );
  CHECK( min < max );

  double const unit = ( *this )( gen );

  double const complement = 1.0 - unit;
  double const left       = min * complement;
  double const right      = max * unit;

  double res = left + right;

  // Floating-point rounding can occasionally produce max even
  // though unit is strictly less than 1.
  if( !( res < max ) ) { res = nextafter( max, min ); }

  // Guard against unexpected rounding outside the lower end-
  // point.
  if( res < min ) { res = min; }

  CHECK( isfinite( res ) );
  CHECK( res >= min );
  CHECK( res < max );

  return res;
}

/****************************************************************
** Helpers.
*****************************************************************/
void check_floating_point_portability() {
  check_portable_rounding_mode();
  check_subnormal_modes();
}

} // namespace rng
