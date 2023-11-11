/****************************************************************
**bytes.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-08.
*
* Description: Type used to represent uninterpreted bytes in the
*              OG's SAV files.
*
*****************************************************************/
#pragma once

// cdr
#include "cdr/converter.hpp"

// base
#include "base/binary-data.hpp"
#include "base/string.hpp"
#include "base/to-str.hpp"

// C++ standard libary
#include <array>

namespace sav {

namespace detail {

cdr::result<std::vector<uint8_t>> bytes_from_canonical_impl(
    cdr::converter& conv, cdr::value const& v, int n_bytes );

}

/****************************************************************
** bytes
*****************************************************************/
// An array of uninterpreted bytes. We have a special wrapper
// type for this so that we can control how it is formatted when
// converted to cdr. The idea that went in to determining that it
// should be formatted as a string instead of as a cdr list of
// numbers is the following: it is anticipated that the rcl/json
// forms of OG sav files will primarily be used for analysis and
// research into the OG, and so, given that JSON does not support
// hex numbers, it was determined that formatting these as a
// string of hex bytes was most human readable during change
// analysis.
template<size_t N>
struct bytes {
  std::array<uint8_t, N> a = {};

  auto operator<=>( bytes const& ) const = default;

  // ADL conversion methods.

  friend void to_str( bytes const& o, std::string& out,
                      base::ADL_t ) {
    for( uint8_t const c : o.a )
      out += fmt::format( "{:02x} ", c );
    if( !out.empty() ) out.resize( out.size() - 1 );
  }

  friend bool read_binary( base::IBinaryIO& b, bytes& o ) {
    for( uint8_t& c : o.a )
      if( !b.read( c ) ) return false;
    return true;
  }

  friend bool write_binary( base::IBinaryIO& b,
                            bytes const&     o ) {
    for( uint8_t const c : o.a )
      if( !b.write( c ) ) return false;
    return true;
  }

  friend cdr::value to_canonical( cdr::converter&,
                                  bytes const& o,
                                  cdr::tag_t<bytes> ) {
    return base::to_str( o );
  }

  friend cdr::result<bytes> from_canonical( cdr::converter& conv,
                                            cdr::value const& v,
                                            cdr::tag_t<bytes> ) {
    UNWRAP_RETURN(
        res, detail::bytes_from_canonical_impl( conv, v, N ) );
    CHECK_EQ( res.size(), N );
    bytes bs;
    std::copy( res.begin(), res.end(), bs.a.begin() );
    return bs;
  }
};

static_assert( cdr::Canonical<bytes<1>> );
static_assert( base::Show<bytes<1>> );
static_assert( base::Binable<bytes<1>> );

} // namespace sav
