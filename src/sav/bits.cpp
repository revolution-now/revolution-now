/****************************************************************
**bits.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-08.
*
* Description: Type used to represent array of bits in the OG's
*              SAV files that we want to format as a string.
*
*****************************************************************/
#include "bits.hpp"

// cdr
#include "cdr/converter.hpp"

// base
#include "base/binary-data.hpp"

using namespace std;

namespace sav {

static_assert( cdr::Canonical<bits<1>> );
static_assert( base::Show<bits<1>> );
static_assert( !base::Binable<bits<1>> );
static_assert( base::Binable<bits<8>> );

/****************************************************************
** bits_base
*****************************************************************/
void bits_base::to_string( std::string& out ) const {
  for( int i = 0; i < int( n_bits ); ++i ) {
    bool const on = n & ( uint64_t{ 1 } << ( n_bits - i - 1 ) );
    out += ( on ? '1' : '0' );
  }
}

bool bits_base::read_binary( base::BinaryData& b ) {
  CHECK( n_bits % 8 == 0 );
  int const nbytes = n_bits / 8;
  n                = 0;
  for( int i = 0; i < nbytes; ++i ) {
    uint8_t byte = 0;
    if( !b.read( byte ) ) return false;
    n |= ( uint64_t{ byte } << ( i * 8 ) );
  }
  return true;
}

bool bits_base::write_binary( base::BinaryData& b ) const {
  CHECK( n_bits % 8 == 0 );
  int const nbytes = n_bits / 8;
  uint64_t  m      = n;
  for( int i = 0; i < nbytes; ++i ) {
    uint8_t const byte = m & 0xff;
    if( !b.write( byte ) ) return false;
    m >>= 8;
  }
  return true;
}

cdr::value bits_base::to_canonical( cdr::converter&,
                                    bits_base const& o ) {
  string out;
  o.to_string( out );
  return out;
}

cdr::result<bits_base> bits_base::from_canonical(
    cdr::converter& conv, cdr::value const& v, int n_bits ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  if( int( str.size() ) != n_bits )
    return conv.err(
        "expected bit string of length {} but found length "
        "{}.",
        n_bits, str.size() );
  uint64_t bs = 0;
  for( int i = 0; i < n_bits; ++i ) {
    if( str[i] != '0' && str[i] != '1' )
      return conv.err(
          "expected bit value '1' or '0' but found '{}'.",
          str[i] );
    bool const on = ( str[i] == '1' );
    bs |= ( ( on ? 1UL : 0UL ) << ( n_bits - i - 1 ) );
  }
  return bits_base{ .n_bits = n_bits, .n = bs };
}

} // namespace sav
