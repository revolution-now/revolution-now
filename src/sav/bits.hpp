/****************************************************************
**bits.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-08.
*
* Description: Type used to represent array of bits in the OG's
*              SAV files that we want to format as a string.
*
*****************************************************************/
#pragma once

// cdr
#include "cdr/ext.hpp"

// base
#include "base/to-str.hpp"

namespace base {
struct BinaryData;
}

namespace sav {

/****************************************************************
** bits_base
*****************************************************************/
struct bits_base {
  int      n_bits = 0; // should be <= 64.
  uint64_t n      = 0;

  auto operator<=>( bits_base const& ) const = default;

 protected:
  void to_string( std::string& out ) const;

  bool read_binary( base::BinaryData& b );

  bool write_binary( base::BinaryData& b ) const;

  static cdr::value to_canonical( cdr::converter&,
                                  bits_base const& o );

  static cdr::result<bits_base> from_canonical(
      cdr::converter& conv, cdr::value const& v, int n_bits );
};

/****************************************************************
** bits
*****************************************************************/
// An array of bits. We have a special wrapper type for this so
// that we can control how it is formatted when converted to cdr.
// The idea that went in to determining that it should be for-
// matted as a string instead of as a cdr list of bools is the
// following: it is anticipated that the rcl/json forms of OG sav
// files will primarily be used for analysis and research into
// the OG, and it was determined that formatting these as a
// string of binary digits was most human readable during change
// analysis.
//
// This is the type we actually use. It just forwards everything
// to the non-templated base so that we can keep the implementa-
// tion out of the header file. But we need this to be templated
// because we need the number of bits to be encoded in the type.
template<size_t N> // number of bits.
requires( N <= 64 )
struct bits : private bits_base {
  bits( uint64_t n )
    : bits_base{ .n_bits = N, .n = clamp_N( n ) } {}
  bits() : bits( 0 ) {}

  uint64_t n() const { return this->bits_base::n; }

  auto operator<=>( bits const& ) const = default;

  friend void to_str( bits const& o, std::string& out,
                      base::ADL_t ) {
    o.to_string( out );
  }

  friend bool read_binary( base::BinaryData& b, bits& o )
  requires( N % 8 == 0 )
  {
    return o.read_binary( b );
  }

  friend bool write_binary( base::BinaryData& b, bits const& o )
  requires( N % 8 == 0 )
  {
    return o.write_binary( b );
  }

  friend cdr::value to_canonical( cdr::converter& conv,
                                  bits const&     o,
                                  cdr::tag_t<bits> ) {
    return bits_base::to_canonical( conv, o );
  }

  friend cdr::result<bits> from_canonical( cdr::converter& conv,
                                           cdr::value const& v,
                                           cdr::tag_t<bits> ) {
    UNWRAP_RETURN( bb, bits_base::from_canonical( conv, v, N ) );
    return bits( bb );
  }

 private:
  bits( bits_base bb ) : bits_base( bb ) {}

  // Ensure that n gets clamped to within values that it is al-
  // lowed to assume given the max number of bits.
  static uint64_t clamp_N( uint64_t n ) {
    if constexpr( N == 0 )
      return 0;
    else
      return n & ( std::numeric_limits<uint64_t>::max() >>
                   ( 64 - N ) );
  }
};

} // namespace sav
