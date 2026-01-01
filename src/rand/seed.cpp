/****************************************************************
**seed.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-01.
*
* Description: Representation for random seeds.
*
*****************************************************************/
#include "seed.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"
#include "cdr/ext.hpp"

// base
#include "base/conv.hpp"

namespace rng {

namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;

size_t constexpr kHashStrSize = 32;

// Parses e.g. 1A4DC031 as a hex 32 bit unsigned int.
maybe<uint32_t> parse8hex( string_view const sv ) {
  if( sv.size() != 8 ) return nothing;
  return base::from_chars<uint32_t>( sv, /*base=*/16 );
}

// NOTE: This should not be changed as it might potentially af-
// fect how seeds and/or random numbers are generated and/or
// evolved and/or interpreted in the game, which will have
// user-facing effects.
void mix128( uint32_t& a, uint32_t& b, uint32_t& c,
             uint32_t& d ) {
  // fmix32 is MurmurHash3’s finalizer (apparently has excellent
  // "avalanche");
  static auto const fmix32 =
      [] [[nodiscard]] ( uint32_t n ) -> uint32_t {
    n ^= n >> 16;
    n *= 0x85ebca6b;
    n ^= n >> 13;
    n *= 0xc2b2ae35;
    n ^= n >> 16;
    return n;
  };

  // First cross-mix.
  a += b;
  c += d;
  b += c;
  d += a;

  a = fmix32( a );
  b = fmix32( b );
  c = fmix32( c );
  d = fmix32( d );

  // Second cross-mix to spread influence everywhere.
  a ^= b;
  c ^= d;
  b ^= c;
  d ^= a;
}

} // namespace

/****************************************************************
** seed
*****************************************************************/
maybe<seed> seed::from_string( string_view const sv ) {
  if( sv.size() != kHashStrSize ) return nothing;
  seed res;
  UNWRAP_RETURN_T( res.e4, parse8hex( sv.substr( 0, 8 ) ) );
  UNWRAP_RETURN_T( res.e3, parse8hex( sv.substr( 8, 8 ) ) );
  UNWRAP_RETURN_T( res.e2, parse8hex( sv.substr( 16, 8 ) ) );
  UNWRAP_RETURN_T( res.e1, parse8hex( sv.substr( 24, 8 ) ) );
  return res;
}

void seed::mix() { mix128( e1, e2, e3, e4 ); }

/****************************************************************
** to_str
*****************************************************************/
void to_str( seed const& o, string& out, base::tag<seed> ) {
  out += format( "{:08x}{:08x}{:08x}{:08x}", o.e4, o.e3, o.e2,
                 o.e1 );
}

/****************************************************************
** CDR
*****************************************************************/
cdr::value to_canonical( cdr::converter&, seed const& o,
                         cdr::tag_t<seed> ) {
  return base::to_str( o );
}

cdr::result<seed> from_canonical( cdr::converter& conv,
                                  cdr::value const& v,
                                  cdr::tag_t<seed> ) {
  UNWRAP_RETURN( hex, conv.from<string>( v ) );
  if( hex.size() != kHashStrSize )
    return conv.err(
        "seed strings must be {}-character hex strings.",
        kHashStrSize );
  auto const seed = seed::from_string( hex );
  if( !seed.has_value() )
    return conv.err(
        "seed strings must be {}-character hex strings with "
        "characters 0-9, a-f, A-F.",
        kHashStrSize );
  return *seed;
}

} // namespace rng
