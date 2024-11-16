/****************************************************************
**resolutions.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-11-02.
*
* Description: Supported logical resolutions.
*
*****************************************************************/
#include "resolutions.hpp"

// refl
#include "refl/query-enum.hpp"

// base
#include "base/keyval.hpp"

using namespace std;

namespace rn {

namespace {

unordered_map<gfx::size, e_resolution> const
    kResolutionReverseSizeMap = [] {
      unordered_map<gfx::size, e_resolution> res;
      for( auto const r : refl::enum_values<e_resolution> )
        res[resolution_size( r )] = r;
      return res;
    }();

}

/****************************************************************
** Public API.
*****************************************************************/
gfx::size resolution_size( e_resolution const r ) {
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
  }
}

vector<gfx::size> const& supported_resolutions() {
  static vector<gfx::size> const v = [] {
    vector<gfx::size> res;
    res.reserve( refl::enum_count<e_resolution> );
    for( e_resolution const r : refl::enum_values<e_resolution> )
      res.push_back( resolution_size( r ) );
    return res;
  }();
  return v;
}

base::maybe<e_resolution> resolution_from_size(
    gfx::size const sz ) {
  return base::lookup( kResolutionReverseSizeMap, sz );
}

} // namespace rn
