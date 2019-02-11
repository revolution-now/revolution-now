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
#include "errors.hpp"
#include "geo-types.hpp"
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
  Vec<T> data{};

public:
  Matrix( W w, H h, Opt<T> init = std::nullopt ) : w_( w ) {
    CHECK( w >= 0_w );
    CHECK( h >= 0_h );
    size_t size = h._ * w._;
    if( init.has_value() )
      data.assign( size, *init );
    else
      data.resize( size );
    CHECK( data.size() == size );
  }

  Matrix( H h, W w, Opt<T> init = std::nullopt )
    : Matrix( w, h, init ) {}
  Matrix( Delta delta, Opt<T> init = std::nullopt )
    : Matrix( delta.w, delta.h, init ) {}

  typed_span<T const, X> operator[]( Y y ) const {
    CHECK( y >= Y{0} && size_t( y._ ) < data.size() );
    return {&data[y._ * w_._], w_._};
  }
  typed_span<T, X> operator[]( Y y ) {
    CHECK( y >= Y{0} && size_t( y._ ) < data.size() );
    return {&data[y._ * w_._], w_};
  }

  T const& operator[]( Coord coord ) const {
    // These subscript operators should do the range checking.
    return ( *this )[coord.y][coord.x];
  };
  T& operator[]( Coord coord ) {
    // These subscript operators should do the range checking.
    return ( *this )[coord.y][coord.x];
  };
};

// Pass as the third template argument to hash map so that we can
// use enum classes as the keys in maps.
struct EnumClassHash {
  template<class T>
  std::size_t operator()( T t ) const {
    return static_cast<std::size_t>( t );
  }
};

// Here "up" means "toward +inf" and "down" means "toward -inf".
ND int round_up_to_nearest_int_multiple( double d, int m );
ND int round_down_to_nearest_int_multiple( double d, int m );

// Get a reference to a value in a map. Since the key may not ex-
// ist, we return an optional. But since we want a reference to
// the object, we return an optional of a reference wrapper,
// since containers can't hold references. I think the reference
// wrapper returned here should only allow const references to be
// extracted.
template<typename MapT, typename KeyT>
ND auto val_safe( MapT const& m, KeyT const& k )
    -> OptCRef<std::remove_reference_t<decltype( m[k] )>> {
  auto found = m.find( k );
  if( found == m.end() ) return std::nullopt;
  return found->second;
}

// Gave up trying to make variadic templates work inside a
// templatized template parameter, so just use auto for the
// return value.
template<typename MapT, typename KeyT>
ND auto& val_or_die( MapT& m, KeyT const& k ) {
  auto found = m.find( k );
  CHECK( found != m.end() );
  return found->second;
}

// Gave up trying to make variadic templates work inside a
// templatized template parameter, so just use auto for the
// return value.
template<typename MapT, typename KeyT>
ND auto const& val_or_die( MapT const& m, KeyT const& k ) {
  auto found = m.find( k );
  CHECK( found != m.end() );
  return found->second;
}

template<typename MapT, typename KeyT>
ND auto val_safe( MapT& m, KeyT const& k )
    -> OptRef<std::remove_reference_t<decltype( m[k] )>> {
  auto found = m.find( k );
  if( found == m.end() ) return std::nullopt;
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

// Does the set contain the given key. If not, returns nullopt.
// If so, returns the iterator to the location.
template<typename ContainerT, typename KeyT>
ND auto has_key( ContainerT const& s, KeyT const& k )
    -> std::optional<decltype( s.find( k ) )> {
  auto it = s.find( k );
  if( it == s.end() ) return std::nullopt;
  return it;
}

// Non-const version.
template<typename ContainerT, typename KeyT>
ND auto has_key( ContainerT& s, KeyT const& k )
    -> std::optional<decltype( s.find( k ) )> {
  auto it = s.find( k );
  if( it == s.end() ) return std::nullopt;
  return it;
}

template<typename ContainerT, typename ElemT>
ND int count( ContainerT& c, ElemT const& e ) {
  return std::count( c.begin(), c.end(), e );
}

} // namespace rn
