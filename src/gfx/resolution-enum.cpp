/****************************************************************
**resolution-enum.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-02.
*
* Description: Enumerates resolutions that may be supported.
*
*****************************************************************/
#include "resolution-enum.hpp"

// refl
#include "refl/query-enum.hpp"

// base
#include "base/keyval.hpp"

using namespace std;

namespace gfx {

namespace {

using ::base::maybe;
using ::refl::enum_count;
using ::refl::enum_values;

unordered_map<size, e_resolution> const
    kResolutionReverseSizeMap = [] {
      unordered_map<size, e_resolution> res;
      for( auto const r : refl::enum_values<e_resolution> )
        res[resolution_size( r )] = r;
      return res;
    }();

} // namespace

/****************************************************************
** e_resolution
*****************************************************************/
size resolution_size( e_resolution const r ) {
  switch( r ) {
    case e_resolution::_640x360:
      return { .w = 640, .h = 360 };
    case e_resolution::_640x400:
      return { .w = 640, .h = 400 };
    case e_resolution::_768x432:
      return { .w = 768, .h = 432 };
    case e_resolution::_576x360:
      return { .w = 576, .h = 360 };
    case e_resolution::_720x450:
      return { .w = 720, .h = 450 };
    case e_resolution::_640x480:
      return { .w = 640, .h = 480 };
    case e_resolution::_852x360:
      return { .w = 852, .h = 360 };
    case e_resolution::_860x360:
      return { .w = 860, .h = 360 };
    case e_resolution::_960x400:
      return { .w = 960, .h = 400 };
  }
}

vector<e_resolution> const& supported_resolutions() {
  static vector<e_resolution> const v = [] {
    vector<e_resolution> res;
    res.reserve( enum_count<e_resolution> );
    for( e_resolution const r : enum_values<e_resolution> )
      res.push_back( r );
    return res;
  }();
  return v;
}

maybe<e_resolution> resolution_from_size( size const sz ) {
  return base::lookup( kResolutionReverseSizeMap, sz );
}

} // namespace gfx
