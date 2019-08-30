/****************************************************************
**util.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Common utilities for all modules.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "coord.hpp"
#include "errors.hpp"
#include "typed-int.hpp"

// base-util
#include "base-util/non-copyable.hpp"

// Abseil
#include "absl/types/span.h"

// C++ standard library
#include <algorithm>
#include <optional>
#include <string_view>
#include <variant>

namespace rn {

// This is a span type with added type safety in that it can
// only be indexed with a particular type.
template<typename T, typename Idx>
class typed_span {
  absl::Span<T> span_;

public:
  typed_span( T* data, LengthType<Idx> size )
    : span_( data, size._ ) {}

  int size() const { return int( span_.size() ); }

  T const& operator[]( Idx idx ) const {
    CHECK( idx >= Idx{0} && idx._ < span_.size() );
    return span_[idx._];
  }

  T& operator[]( Idx idx ) {
    CHECK( idx >= Idx{0} && size_t( idx._ ) < span_.size() );
    return span_[idx._];
  }
};

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

  Delta size() const {
    using coord_underlying_t = decltype( w_._ );
    if( data_.size() == 0 ) return {};
    return Delta{w_,
                 H{coord_underlying_t( data_.size() / w_._ )}};
  }

  Rect rect() const { return Rect::from( Coord{}, size() ); }

  typed_span<T const, X> operator[]( Y y ) const {
    CHECK( y >= Y{0} && size_t( y._ ) < data_.size() );
    return {&data_[y._ * w_._], w_};
  }
  typed_span<T, X> operator[]( Y y ) {
    CHECK( y >= Y{0} && size_t( y._ ) < data_.size() );
    return {&data_[y._ * w_._], w_};
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

// Here "up" means "toward +inf" and "down" means "toward -inf".
ND int round_up_to_nearest_int_multiple( double d, int m );
ND int round_down_to_nearest_int_multiple( double d, int m );

// Gave up trying to make variadic templates work inside a
// templatized template parameter, so just use auto for the
// return value.
template<typename MapT, typename KeyT>
ND auto& val_or_die( MapT&& m, KeyT const& k ) {
  auto found = FWD( m ).find( k );
  CHECK( found != FWD( m ).end() );
  return found->second;
}

template<typename T, typename... Vs>
auto const& val_or_die( std::variant<Vs...> const& v ) {
  CHECK( std::holds_alternative<T>( v ) );
  return std::get<T>( v );
}

template<typename T, typename... Vs>
auto& val_or_die( std::variant<Vs...>& v ) {
  CHECK( std::holds_alternative<T>( v ) );
  return std::get<T>( v );
}

template<typename T>
auto const& val_or_die( std::optional<T> const& o ) {
  CHECK( o.has_value() );
  return o.value();
}

template<typename T>
auto& val_or_die( std::optional<T>& o ) {
  CHECK( o.has_value() );
  return o.value();
}

// FIXME: move to base-util.
template<typename ContainerT, typename ElemT>
ND int count( ContainerT&& c, ElemT&& e ) {
  return std::count( std::forward<ContainerT>( c ).begin(),
                     std::forward<ContainerT>( c ).end(),
                     std::forward<ElemT>( e ) );
}

template<typename Base, typename From, typename To>
void copy_common_base_object( From const& from, To& to ) {
  *( static_cast<Base*>( &to ) ) =
      *( static_cast<Base const*>( &from ) );
}

// All the parameters must be either the result type or convert-
// ible to the result type.
template<typename Res, typename... T>
auto params_to_vector( T&&... ts ) {
  std::vector<Res> res;
  res.reserve( sizeof...( T ) );
  ( res.push_back( std::forward<T>( ts ) ), ... );
  return res;
}

// These will demangle a type or symbol (e.g. one returned from
// type_id(<type>).name() if the compiler supports it, otherwise
// will return the mangled version.

std::string demangle( char const* name );

// You need to include <typeinfo> in your module to call this.
template<typename T>
std::string demangled_typename() {
  return demangle( typeid( T ).name() );
}

template<typename... Types>
std::string type_list_to_names() {
  std::string joiner = ",";
  auto res = ( ( demangled_typename<Types>() + joiner ) + ... );
  if( res.size() > 0 ) res.resize( res.size() - joiner.size() );
  return res;
}

// If the HOME environment variable is set then return it.
Opt<fs::path> user_home_folder();

} // namespace rn
