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
#include "aliases.hpp"
#include "coord.hpp"
#include "errors.hpp"
#include "fb.hpp"
#include "fmt-helper.hpp"
#include "strong-span.hpp"

// base-util
#include "base-util/non-copyable.hpp"

// C++ standard library
#include <vector>

namespace rn {

// This is a matrix with type-safe indexing using X/Y and where
// the dimensions are set with W/H. Indexing it once returns a
// span that can only be indexed by X; indexing that span will
// return a value.
template<typename T>
class Matrix : public util::movable_only {
  W      w_ = 0_w;
  Vec<T> data_{};

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
};

namespace serial {

template<typename Hint, typename T>
auto serialize( FBBuilder& builder, Matrix<T> const& m,
                serial::ADL ) {
  auto size   = m.size();
  auto s_size = serialize<fb_serialize_hint_t<decltype(
      std::declval<Hint>().size() )>>( builder, size,
                                       serial::ADL{} );

  std::vector<std::pair<Coord, T>> data;
  data.reserve( size_t( size.area() ) );
  for( auto const& c : m.rect() ) data.emplace_back( c, m[c] );
  auto s_data = serialize<fb_serialize_hint_t<decltype(
      std::declval<Hint>().data() )>>( builder, data,
                                       serial::ADL{} );

  return ReturnValue{ Hint::Traits::Create(
      builder, s_size.get(), s_data.get() ) };
}

template<typename SrcT, typename T>
expect<> deserialize( SrcT const* src, Matrix<T>* m,
                      serial::ADL ) {
  // SrcT should be a table with a `size` field of type Delta and
  // a `data` field which is a flatbuffers Vector of a pair-like
  // object containing a Coord and the matrix element itself.
  if( src == nullptr ) {
    // `dst` should be in its default-constructed state, which is
    // an empty map.
    return xp_success_t{};
  }

  UNXP_CHECK( src->size() != nullptr );
  UNXP_CHECK( src->data() != nullptr );

  Delta size;
  XP_OR_RETURN_(
      deserialize( src->size(), &size, serial::ADL{} ) );

  UNXP_CHECK( ( size.area() == 0 ) ==
              ( src->data()->size() == 0 ) );

  if( size.area() == 0 ) return xp_success_t{};

  std::vector<std::pair<Coord, T>> data;
  XP_OR_RETURN_(
      deserialize( src->data(), &data, serial::ADL{} ) );

  *m = Matrix<T>( size );
  FlatSet<Coord> seen;
  seen.reserve( size.area() );
  for( auto& [coord, elem] : data ) {
    ( *m )[coord] = std::move( elem );
    seen.insert( coord );
  }

  // Make sure we got all unique coords.
  UNXP_CHECK( int( seen.size() ) == size.area() );

  return xp_success_t{};
}

} // namespace serial

} // namespace rn

namespace fmt {

template<typename T>
struct formatter<::rn::Matrix<T>> : formatter_base {
  template<typename FormatContext>
  auto format( ::rn::Matrix<T> const& o, FormatContext& ctx ) {
    return formatter_base::format(
        fmt::format( "Matrix{{size={}}}", o.size() ), ctx );
  }
};

} // namespace fmt
