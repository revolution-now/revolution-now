/****************************************************************
**matrix.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-22.
*
* Description: The Matrix.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "error.hpp"
#include "strong-span.hpp"

// refl
#include "refl/cdr.hpp"

// Cdr
#include "cdr/converter.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext.hpp"

// base
#include "base/fmt.hpp"

// C++ standard library
#include <vector>

namespace rn {

// This is a matrix with type-safe indexing using X/Y and where
// the dimensions are set with W/H. Indexing it once returns a
// span that can only be indexed by X; indexing that span will
// return a value.
template<typename T>
class Matrix {
  W              w_ = 0_w;
  std::vector<T> data_{};

 public:
  Matrix( W w, H h ) : w_( w ) {
    CHECK( w >= 0_w );
    CHECK( h >= 0_h );
    size_t size = h._ * w._;
    data_.resize( size );
    CHECK( data_.size() == size );
  }

  Matrix( W w, H h, T init ) : w_( w ) {
    CHECK( w >= 0_w );
    CHECK( h >= 0_h );
    size_t size = h._ * w._;
    data_.assign( size, init );
    CHECK( data_.size() == size );
  }

  Matrix( H h, W w, T init ) : Matrix( w, h, init ) {}
  Matrix( H h, W w ) : Matrix( w, h ) {}
  Matrix( Delta delta ) : Matrix( delta.w, delta.h ) {}
  Matrix( Delta delta, T init )
    : Matrix( delta.w, delta.h, init ) {}
  Matrix() : Matrix( Delta{} ) {}

  Matrix( std::vector<T>&& data, W w )
    : w_{ w }, data_{ std::move( data ) } {}

  Matrix( Matrix const& ) = default;
  Matrix& operator=( Matrix const& ) = default;
  Matrix( Matrix&& )                 = default;
  Matrix& operator=( Matrix&& ) = default;

  bool operator==( Matrix<T> const& rhs ) const {
    return w_ == rhs.w_ && data_ == rhs.data_;
  }
  bool operator!=( Matrix<T> const& rhs ) const {
    return !( *this == rhs );
  }

  Delta size() const {
    using coord_underlying_t = decltype( w_._ );
    if( data_.size() == 0 ) return {};
    return Delta{
        w_, H{ coord_underlying_t( data_.size() / w_._ ) } };
  }

  Rect rect() const { return Rect::from( Coord{}, size() ); }

  strong_span<T const, X, W> operator[]( Y y ) const {
    CHECK( y >= Y{ 0 } && size_t( y._ ) < data_.size() );
    return { &data_[y._ * w_._], w_ };
  }
  strong_span<T, X, W> operator[]( Y y ) {
    CHECK( y >= Y{ 0 } && size_t( y._ ) < data_.size() );
    return { &data_[y._ * w_._], w_ };
  }

  T const& operator[]( Coord coord ) const {
    // These subscript operators should do the range checking.
    return ( *this )[coord.y][coord.x];
  };

  T& operator[]( Coord coord ) {
    // These subscript operators should do the range checking.
    return ( *this )[coord.y][coord.x];
  };

  void clear() {
    data_.clear();
    w_ = 0_w;
  }

  std::vector<T> const& data() const { return data_; }

  friend void to_str( Matrix const& o, std::string& out,
                      base::ADL_t ) {
    out += fmt::format( "Matrix{{size={}}}", o.size() );
  }
};

/****************************************************************
** Cdr
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
    for( Coord c : m.rect() ) {
      T const&       elem = m[c];
      static const T def{};
      if( elem == def ) continue;
      lst.emplace_back( conv.to( std::pair{ c, m[c] } ) );
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
  std::unordered_set<std::string> used_keys;
  UNWRAP_RETURN(
      has_coords,
      conv.from_field<bool>( tbl, "has_coords", used_keys ) );
  UNWRAP_RETURN(
      size, conv.from_field<Delta>( tbl, "size", used_keys ) );

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
        data, conv.from_field<std::vector<std::pair<Coord, T>>>(
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

} // namespace rn
