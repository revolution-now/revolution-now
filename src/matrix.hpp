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

  MOVABLE_ONLY( Matrix );

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

  friend void to_str( Matrix const& o, std::string& out,
                      base::ADL_t ) {
    out += fmt::format( "Matrix{{size={}}}", o.size() );
  }
};

template<cdr::ToCanonical T>
cdr::value to_canonical( cdr::converter&  conv,
                         Matrix<T> const& m,
                         cdr::tag_t<Matrix<T>> ) {
  cdr::table tbl;
  conv.to_field( tbl, "size", m.size() );
  bool do_not_write_defaults =
      !conv.opts().write_fields_with_default_value;
  std::vector<std::pair<Coord, T>> data;
  data.reserve( m.size().area() );
  for( Coord c : m.rect() ) {
    T const&       elem = m[c];
    static const T def{};
    if( do_not_write_defaults && elem == def ) continue;
    data.emplace_back( c, elem );
  }
  conv.to_field( tbl, "data", data );
  return cdr::value{ std::move( tbl ) };
}

template<cdr::FromCanonical T>
cdr::result<Matrix<T>> from_canonical( cdr::converter&   conv,
                                       cdr::value const& v,
                                       cdr::tag_t<Matrix<T>> ) {
  UNWRAP_RETURN( tbl, conv.ensure_type<cdr::table>( v ) );
  std::unordered_set<std::string> used_keys;
  UNWRAP_RETURN(
      size, conv.from_field<Delta>( tbl, "size", used_keys ) );
  UNWRAP_RETURN(
      data, conv.from_field<std::vector<std::pair<Coord, T>>>(
                tbl, "data", used_keys ) );

  bool allow_missing_coords =
      conv.opts().default_construct_missing_fields;

  if( size.area() < int( data.size() ) )
    return conv.err(
        "serialized matrix has more coordinates in 'data' ({}) "
        "then are allowed by the 'size' ({}).",
        data.size(), size.area() );

  if( !allow_missing_coords )
    if( size.area() != int( data.size() ) )
      return conv.err(
          "inconsistent sizes between 'size' field and 'data' "
          "field ('size' implies {} while 'data' implies {}).",
          size.area(), data.size() );

  Matrix<T> res( size );
  for( auto& [coord, elem] : data )
    res[coord] = std::move( elem );

  // TODO: figure out if implicit move would apply here only is-
  // suing a `return res`.
  return cdr::result<Matrix<T>>( std::move( res ) );
}

} // namespace rn
