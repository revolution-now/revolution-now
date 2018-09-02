/****************************************************************
* base-util.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Common utilities for all modules.
*
*****************************************************************/
#pragma once

#include "aliases.hpp"
#include "typed-int.hpp"

#include <istream>
#include <optional>
#include <ostream>
#include <string_view>

namespace rn {

struct Rect {
  X x; Y y; W w; H h;

  // Useful for generic code; allows referencing a coordinate
  // from the type.
  template<typename Dimension>
  Dimension& coordinate();

  // Useful for generic code; allows referencing a width/height
  // from the of the associated dimension, i.e., with Dimension=X
  // it will return the width of type (W).
  template<typename Dimension>
  LengthType<Dimension>& length();

  // New coord equal to this one unit of edge trimmed off
  // on all sides.  That is, we will have:
  //
  //   (width,height) ==> (width-2,height-2)
  //
  // unless one of the dimensions is initially 1 or 0 in
  // which case that dimension will be 0 in the result.
  //
  // For the (x,y) coordinates we will always have:
  //
  //   (x,y) ==> (x+1,y+1)
  //
  // unless one of the dimensions has width 0 in which case
  // that dimension will remain as-is.
  Rect edges_removed();
};

enum class direction {
  nw, n, ne,
  w,  c, e,
  sw, s, se
};

struct Coord {
  Y y; X x;

  // Useful for generic code; allows referencing a coordinate
  // from the type.
  template<typename Dimension>
  Dimension& coordinate();

  bool operator==( Coord const& other ) const {
    return (y == other.y) && (x == other.x);
  }

  bool operator!=( Coord const& other ) const {
    return !(*this == other);
  }

  Coord moved( direction d );
  bool is_inside( Rect const& rect );
};

using OptCoord = std::optional<Coord>;

struct Delta {
  W w; H h;
  bool operator==( Delta const& other ) const {
    return (h == other.h) && (w == other.w);
  }
};

std::ostream& operator<<( std::ostream& out, Rect const& r );

void die( char const* file, int line, std::string_view msg );

// Pass as the third template argument to hash map so that we can
// use enum classes as the keys in maps.
struct EnumClassHash {
  template<class T>
  std::size_t operator()( T t ) const {
    return static_cast<std::size_t>( t );
  }
};

int round_up_to_nearest_int_multiple( double d, int m );
int round_down_to_nearest_int_multiple( double d, int m );

// Get a reference to a value in a map. Since the key may not ex-
// ist, we return an optional. But since we want a reference to
// the object, we return an optional of a reference wrapper,
// since containers can't hold references. I think the reference
// wrapper returned here should only allow const references to be
// extracted.
template<
    typename KeyT,
    typename ValT,
    template<typename KeyT_, typename ValT_>
    typename MapT
>
OptCRef<ValT> get_val_safe( MapT<KeyT,ValT> const& m,
                            KeyT            const& k ) {
    auto found = m.find( k );
    if( found == m.end() )
        return std::nullopt;
    return found->second;
}

// Non-const version.
template<
    typename KeyT,
    typename ValT,
    template<typename KeyT_, typename ValT_>
    typename MapT
>
OptRef<ValT> get_val_safe( MapT<KeyT,ValT>& m,
                           KeyT      const& k ) {
    auto found = m.find( k );
    if( found == m.end() )
        return std::nullopt;
    return found->second;
}

// Does the set contain the given key.
template<typename ContainerT, typename KeyT>
bool has_key( ContainerT const& s, KeyT const& k ) {
    return s.find( k ) != s.end();
}

} // namespace rn
