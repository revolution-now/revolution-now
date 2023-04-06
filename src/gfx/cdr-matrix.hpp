/****************************************************************
**cdr-matrix.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-22.
*
* Description: Cdr conversions for gfx::Matrix.
*
*****************************************************************/
#pragma once

// gfx
#include "iter.hpp"
#include "matrix.hpp"

// refl
#include "refl/cdr.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext.hpp"

namespace gfx {

/****************************************************************
** gfx::Matrix cdr conversions.
*****************************************************************/
// The Matrix<T> class, although generic, happens to be used to
// represent the world map in the game. Since this map can get
// very large, performance is an issue here, at least in debug
// builds. For this reason, the Cdr conversion for Matrix is a
// bit more complicated than it could be because it needs to be
// as efficient as possible with regard to how much data it is
// converting (it needs to convert as few cells as possible).
//
// to_canonical:
//
//   For conversion to Cdr we check if the options request
//   writing default-valued fields and, if so, we choose to
//   simply write the matrix elements into a list of T without
//   any coordinates for each cell, since there is never an ambi-
//   guity since all cells are written.
//
//   On the other hand if we are not writing default-avlued
//   fields, then we will take advantage of that and write a
//   sparse list. That means that we will be missing some cells
//   in the list, so therefore instead of writing a list of T, we
//   need to write a list of pair<Coord, T> so that
//   from_canonical knows which cell each element corresponds to.
//   Writing the coordinates for each cell adds overhead, but if
//   the map is land-sparse then this would still be advan-
//   tageious.
//
//   Which of the above methods was chosen is recorded into a
//   field called `has_coords`.
//
// from_canonical:
//
//   This simply reads the value of `has_coords` and then deseri-
//   alizes accordingly.
//
// As for which of the above two methods will be more efficient,
// it depends on how big the map is and many default-valued map
// squares there are. If the map is small, then it probably
// doesn't matter. If the map is large however, then it comes
// down to how many default water squares there are (a standard
// water square with nothing in it and no attributes should defi-
// nitely have a default value; if it doesn't, then that is a
// waste). If there are many water squares then the sparse ap-
// proach should probably be used, otherwise the non-sparse will
// be faster.
//
template<cdr::ToCanonical T>
cdr::value to_canonical( cdr::converter&  conv,
                         Matrix<T> const& m,
                         cdr::tag_t<Matrix<T>> ) {
  bool write_defaults =
      conv.opts().write_fields_with_default_value;
  if( !write_defaults && m == Matrix<T>{} ) return cdr::table{};
  cdr::table tbl;
  conv.to_field( tbl, "size", m.size() );
  cdr::list lst;
  lst.reserve( m.data().size() );
  if( !write_defaults ) {
    conv.to_field( tbl, "has_coords", true );
    rect_iterator const ri( m.rect() );
    for( rn::Coord const coord : ri ) {
      T const&       elem = m[coord];
      static const T def{};
      if( elem == def ) continue;
      lst.emplace_back(
          conv.to( std::pair{ coord, m[coord] } ) );
    }
  } else {
    // Write defaults.
    conv.to_field( tbl, "has_coords", false );
    std::vector<T> const& data = m.data();
    int                   size = int( data.size() );
    for( int i = 0; i < size; ++i )
      lst.emplace_back( conv.to( data[i] ) );
  }

  if( write_defaults || !lst.empty() )
    tbl["data"] = cdr::value{ std::move( lst ) };
  return cdr::value{ std::move( tbl ) };
}

template<cdr::FromCanonical T>
cdr::result<Matrix<T>> from_canonical( cdr::converter&   conv,
                                       cdr::value const& v,
                                       cdr::tag_t<Matrix<T>> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  std::set<std::string> used_keys;
  UNWRAP_RETURN(
      has_coords,
      conv.from_field<bool>( tbl, "has_coords", used_keys ) );
  UNWRAP_RETURN( size, conv.from_field<rn::Delta>( tbl, "size",
                                                   used_keys ) );

  bool allow_missing_coords =
      conv.opts().default_construct_missing_fields;

  auto check_sizes =
      [&]( auto const& data ) -> base::valid_or<cdr::error> {
    if( size.area() < int( data.size() ) )
      return conv.err(
          "serialized matrix has more coordinates in 'data' "
          "({}) then are allowed by the 'size' ({}).",
          data.size(), size.area() );

    if( !allow_missing_coords )
      if( size.area() != int( data.size() ) )
        return conv.err(
            "inconsistent sizes between 'size' field and 'data' "
            "field ('size' implies {} while 'data' implies {}).",
            size.area(), data.size() );
    return base::valid;
  };

  if( has_coords ) {
    UNWRAP_RETURN(
        data,
        conv.from_field<std::vector<std::pair<rn::Coord, T>>>(
            tbl, "data", used_keys ) );
    HAS_VALUE_OR_RET( check_sizes( data ) );
    Matrix<T> res( size );
    for( auto& [coord, elem] : data )
      res[coord] = std::move( elem );
    HAS_VALUE_OR_RET(
        conv.end_field_tracking( tbl, used_keys ) );
    return cdr::result<Matrix<T>>( std::move( res ) );
  } else {
    UNWRAP_RETURN( data, conv.from_field<std::vector<T>>(
                             tbl, "data", used_keys ) );
    HAS_VALUE_OR_RET( check_sizes( data ) );
    HAS_VALUE_OR_RET(
        conv.end_field_tracking( tbl, used_keys ) );
    return cdr::result<Matrix<T>>(
        Matrix<T>( std::move( data ), size.w ) );
  }
}

} // namespace gfx
