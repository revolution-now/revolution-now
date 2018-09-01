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

struct Coord {
  Y y; X x;
  bool operator==( Coord const& other ) const {
    return (y == other.y) && (x == other.x);
  }
};
using OptCoord = std::optional<Coord>;

struct Delta {
  W w; H h;
  bool operator==( Delta const& other ) const {
    return (h == other.h) && (w == other.w);
  }
};

struct Rect {
  X x; Y y; W w; H h;
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

} // namespace rn
