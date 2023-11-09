/****************************************************************
**bytes.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-08.
*
* Description: Type used to represent uninterpreted bytes in the
*              OG's SAV files.
*
*****************************************************************/
#include "bytes.hpp"

using namespace std;

namespace sav {

namespace {

cdr::result<uint8_t> parse_nibbles( cdr::converter& conv,
                                    int             idx,
                                    string const& two_nibbles ) {
  if( two_nibbles.size() != 2 )
    return conv.err(
        "expected a two-digit hex byte at idx {} but instead "
        "found a token of length {}.",
        idx, two_nibbles.size() );
  size_t num_chars_parsed = 0;
  auto constexpr HEX_BASE = 16;
  unsigned long res =
      std::stoul( two_nibbles, &num_chars_parsed, HEX_BASE );
  if( num_chars_parsed != 2 )
    return conv.err( "failed to parse hex byte 0x{}.",
                     two_nibbles );
  CHECK_LE( res, 255u );
  return res;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
namespace detail {

cdr::result<vector<uint8_t>> bytes_from_canonical_impl(
    cdr::converter& conv, cdr::value const& v, int n_bytes ) {
  UNWRAP_RETURN( str, conv.ensure_type<string>( v ) );
  // We're parsing a sequence of hex bytes: "ff 00 12 a3 c4".
  int const kExpectedStrSize = std::max( n_bytes * 3 - 1, 0 );
  if( int( str.size() ) != kExpectedStrSize )
    return conv.err(
        "expected string with length equal to {}, but instead "
        "found length {}.",
        kExpectedStrSize, str.size() );
  if( n_bytes == 0 ) return vector<uint8_t>{};
  vector<string> const split = base::str_split( str, ' ' );
  if( int( split.size() ) != n_bytes )
    return conv.err(
        "expected {} components when splitting string on "
        "spaces, instead found {}.",
        n_bytes, split.size() );
  vector<uint8_t> res;
  res.reserve( n_bytes );
  for( int idx = 0; string const& two_nibbles : split ) {
    UNWRAP_RETURN( n,
                   parse_nibbles( conv, idx++, two_nibbles ) );
    res.push_back( n );
  }
  return res;
}

} // namespace detail

} // namespace sav
