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

using namespace std;

namespace rn {

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

} // namespace rn
