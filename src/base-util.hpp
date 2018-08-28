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

#include <string_view>

namespace rn {

struct Rect {
  int x, y, w, h;
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

} // namespace rn
