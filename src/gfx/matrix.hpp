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

// Revolution Now

// gfx
#include "coord.hpp"

// base
#include "base/attributes.hpp"
#include "base/error.hpp"
#include "base/fmt.hpp"

// C++ standard library
#include <span>
#include <vector>

namespace gfx {

// This is a matrix with type-safe indexing using X/Y and where
// the dimensions are set with W/H. Indexing it once returns a
// span that can only be indexed by X; indexing that span will
// return a value.
template<typename T>
struct Matrix {
 private:
  int            w_ = 0;
  std::vector<T> data_{};

 public:
  Matrix( rn::Delta delta ) : w_( delta.w ) {
    CHECK( delta.w >= 0 );
    CHECK( delta.h >= 0 );
    size_t size = delta.h * delta.w;
    data_.resize( size );
    CHECK( data_.size() == size );
  }

  Matrix( rn::Delta delta, T init ) : w_( delta.w ) {
    CHECK( delta.w >= 0 );
    CHECK( delta.h >= 0 );
    size_t size = delta.h * delta.w;
    data_.assign( size, init );
    CHECK( data_.size() == size );
  }

  Matrix() : Matrix( rn::Delta{} ) {}

  Matrix( std::vector<T>&& data, int w )
    : w_{ w }, data_{ std::move( data ) } {}

  bool operator==( Matrix<T> const& rhs ) const {
    return w_ == rhs.w_ && data_ == rhs.data_;
  }

  rn::Delta size() const {
    if( data_.size() == 0 ) return {};
    return rn::Delta{ .w = w_, .h = int( data_.size() / w_ ) };
  }

  rn::Rect rect() const {
    return rn::Rect::from( rn::Coord{}, size() );
  }

  std::span<T const> operator[]( int y ) const
      ATTR_LIFETIMEBOUND {
    CHECK( y >= 0 && size_t( y ) < data_.size() );
    return { &data_[y * w_], size_t( w_ ) };
  }

  std::span<T> operator[]( int y ) ATTR_LIFETIMEBOUND {
    CHECK( y >= 0 && size_t( y ) < data_.size() );
    return { &data_[y * w_], size_t( w_ ) };
  }

  T const& operator[]( rn::Coord coord ) const
      ATTR_LIFETIMEBOUND {
    // These subscript operators should do the range checking.
    return ( *this )[coord.y][coord.x];
  };

  T& operator[]( rn::Coord coord ) ATTR_LIFETIMEBOUND {
    // These subscript operators should do the range checking.
    return ( *this )[coord.y][coord.x];
  };

  void clear() {
    data_.clear();
    w_ = 0;
  }

  std::vector<T> const& data() const { return data_; }

  friend void to_str( Matrix const& o, std::string& out,
                      base::ADL_t ) {
    out += fmt::format( "Matrix{{size={}}}", o.size() );
  }
};

} // namespace gfx
