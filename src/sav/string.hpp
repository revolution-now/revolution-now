/****************************************************************
**string.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-07.
*
* Description: Type used to represent strings in the OG's SAV
*              files.
*
*****************************************************************/
#pragma once

// cdr
#include "cdr/converter.hpp"

// base
#include "base/binary-data.hpp"
#include "base/to-str.hpp"

// C++ standard libary
#include <array>

namespace sav {

/****************************************************************
** array_string
*****************************************************************/
// A string backed by a fixed size array. The string will have a
// null zero terminator if and only if the length of the string
// is shorter than the buffer.
template<size_t N>
struct array_string {
  auto operator<=>( array_string const& ) const = default;

  std::array<unsigned char, N> a = {};
};

// to_str
template<size_t N>
void to_str( array_string<N> const& o, std::string& out,
             base::ADL_t ) {
  for( unsigned char const c : o.a ) {
    if( c == 0 ) break;
    out += c;
  }
}

// Binary conversions.
template<size_t N>
bool read_binary( base::BinaryData& b, array_string<N>& o ) {
  for( unsigned char& c : o.a )
    if( !b.read( c ) ) return false;
  return true;
}

template<size_t N>
bool write_binary( base::BinaryData&      b,
                   array_string<N> const& o ) {
  for( unsigned char const c : o.a )
    if( !b.write( c ) ) return false;
  return true;
}

// Cdr conversions.
template<size_t N>
cdr::value to_canonical( cdr::converter&,
                         array_string<N> const& o,
                         cdr::tag_t<array_string<N>> ) {
  return base::to_str( o );
}

template<size_t N>
cdr::result<array_string<N>> from_canonical(
    cdr::converter& conv, cdr::value const& v,
    cdr::tag_t<array_string<N>> ) {
  UNWRAP_RETURN( str, conv.ensure_type<std::string>( v ) );
  if( str.size() > N )
    return conv.err(
        "expected string with length <= {}, but instead found "
        "length {}.",
        N, str.size() );
  array_string<N> res;
  if constexpr( N > 0 ) { CHECK( res.a[0] == 0 ); }
  int idx = 0;
  for( auto const c : str ) {
    CHECK_LT( idx, int( N ) );
    res.a[idx++] = c;
  }
  // The remainder should be left as zeroes.
  return res;
}

} // namespace sav
