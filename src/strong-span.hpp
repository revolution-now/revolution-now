/****************************************************************
**strong-span.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-22.
*
* Description: Span type with strongly-typed indexes.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "errors.hpp"

// C++ standard library
#include <span>

namespace rn {

// This is a span type with added type safety in that it can
// only be indexed with a particular type.
template<typename T, typename Idx, typename Length>
class strong_span {
  std::span<T> span_;

public:
  strong_span( T* data, Length size ) : span_( data, size._ ) {}

  int size() const { return int( span_.size() ); }

  auto begin() { return std::begin( span_ ); }
  auto cbegin() const { return std::cbegin( span_ ); }
  auto end() { return std::end( span_ ); }
  auto cend() const { return std::cend( span_ ); }

  T const& operator[]( Idx idx ) const {
    CHECK( idx >= Idx{ 0 } && size_t( idx._ ) < span_.size() );
    return span_[idx._];
  }

  T& operator[]( Idx idx ) {
    CHECK( idx >= Idx{ 0 } && size_t( idx._ ) < span_.size() );
    return span_[idx._];
  }
};

} // namespace rn
