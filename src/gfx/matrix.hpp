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

// luapp
#include "luapp/ext-userdata.hpp"
#include "luapp/ext-usertype.hpp"
#include "luapp/state.hpp"

// traverse
#include "traverse/ext.hpp"
#include "traverse/type-ext.hpp"

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
  int w_ = 0;
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

  Matrix( Matrix&& )                 = default;
  Matrix( Matrix const& )            = default;
  Matrix& operator=( Matrix&& )      = default;
  Matrix& operator=( Matrix const& ) = default;

  Matrix( std::vector<T>&& data, int w )
    : w_{ w }, data_{ std::move( data ) } {}

  void reset( rn::Delta size ) { *this = Matrix( size ); }

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
    CHECK( y >= 0 && y < size().h );
    return { &data_[y * w_], size_t( w_ ) };
  }

  std::span<T> operator[]( int y ) ATTR_LIFETIMEBOUND {
    CHECK( y >= 0 && y < size().h );
    return { &data_[y * w_], size_t( w_ ) };
  }

  T const& operator[]( point point ) const ATTR_LIFETIMEBOUND {
    // These subscript operators should do the range checking.
    return ( *this )[point.y][point.x];
  };

  T& operator[]( point point ) ATTR_LIFETIMEBOUND {
    // These subscript operators should do the range checking.
    return ( *this )[point.y][point.x];
  };

  void clear() {
    data_.clear();
    w_ = 0;
  }

  std::vector<T> const& data() const { return data_; }

  friend void to_str( Matrix const& o, std::string& out,
                      base::tag<Matrix> ) {
    out += fmt::format( "Matrix{{size={}}}", o.size() );
  }

  // Implement trv::Traversable.
  friend void traverse( Matrix const& o, auto& fn,
                        trv::tag_t<Matrix> ) {
    using namespace std::literals;
    auto const y_size = o.size().h;
    gfx::point p;
    for( p.y = 0; p.y < y_size; ++p.y )
      for( p.x = 0; p.x < o.w_; ++p.x ) //
        fn( o[p], p );
  }

  template<typename U>
  static void define_usertype_for_template( lua::state& st ) {
    // NOTE: DO NOT use T in this function, only U.

    auto u = st.usertype.create<U>();

    // TODO: may need to expand this in the future.

    u["size"] = [&]( U& o ) -> lua::table {
      lua::table tbl     = st.table.create();
      gfx::size const sz = o.size();
      tbl["w"]           = sz.w;
      tbl["h"]           = sz.h;
      return tbl;
    };
  }

  friend void define_usertype_for( lua::state& st,
                                   lua::tag<Matrix<T>> ) {
    define_usertype_for_template<Matrix<T>>( st );
  }
};

} // namespace gfx

/****************************************************************
** TypeTraverse Specializations.
*****************************************************************/
namespace trv {
TRV_TYPE_TRAVERSE( ::gfx::Matrix, T );
}

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS_T( ::gfx::Matrix, owned_by_cpp ){};
}
