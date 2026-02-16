/****************************************************************
**map-matrix.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-02-16.
*
* Description: Type that makes it easier to fwd decl. the matrix
*              that holds the map.
*
*****************************************************************/
#include "map-matrix.hpp"

// gfx
#include "gfx/cdr-matrix.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

namespace rn {

namespace {

using namespace std;

} // namespace

/****************************************************************
** MapMatrix.
*****************************************************************/
// Implement cdr::ToCanonical.
cdr::value to_canonical( cdr::converter& conv,
                         MapMatrix const& m,
                         cdr::tag_t<MapMatrix> ) {
  return to_canonical( conv, m.as_base(),
                       cdr::tag_t<gfx::Matrix<MapSquare>>{} );
}

// Implement cdr::FromCanonical.
cdr::result<MapMatrix> from_canonical( cdr::converter& conv,
                                       cdr::value const& v,
                                       cdr::tag_t<MapMatrix> ) {
  UNWRAP_RETURN(
      res, from_canonical(
               conv, v, cdr::tag_t<gfx::Matrix<MapSquare>>{} ) );
  return MapMatrix( std::move( res ) );
}

} // namespace rn
