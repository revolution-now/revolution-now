/****************************************************************
**plane.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-30.
*
* Description: Basic unit of game interface.
*
*****************************************************************/
#include "plane.hpp"

// Revolution Now
#include "input.hpp"

// gfx
#include "gfx/resolution-enum.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/query-enum.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::e_resolution;
using ::gfx::size;
using ::refl::enum_values;

}

/****************************************************************
** IPlane
*****************************************************************/
IPlane::e_accept_drag IPlane::can_drag(
    input::e_mouse_button /*unused*/, Coord /*unused*/ ) {
  return e_accept_drag::no;
}

void IPlane::draw( rr::Renderer&, Coord ) const {}

void IPlane::on_drag( input::mod_keys const& /*unused*/,
                      input::e_mouse_button /*unused*/,
                      Coord /*unused*/, Coord /*unused*/,
                      Coord /*unused*/ ) {}

void IPlane::on_drag_finished( input::mod_keys const& /*unused*/,
                               input::e_mouse_button /*unused*/,
                               Coord /*unused*/,
                               Coord /*unused*/ ) {}

void IPlane::on_logical_resolution_selected(
    gfx::e_resolution ) {}

static gfx::e_resolution on_logical_resolution_changed_impl(
    IPlane const& plane, gfx::e_resolution const resolution ) {
  if( plane.supports_resolution( resolution ) )
    // Fast path.
    return resolution;
  e_resolution res       = resolution;
  int best_area_delta    = numeric_limits<int>::max();
  size const actual_size = gfx::resolution_size( resolution );
  for( auto const r : enum_values<e_resolution> ) {
    if( !plane.supports_resolution( r ) ) continue;
    size const r_size = resolution_size( r );
    if( !r_size.fits_inside( actual_size ) ) continue;
    int const area_delta = actual_size.area() - r_size.area();
    if( area_delta >= best_area_delta ) continue;
    res             = r;
    best_area_delta = area_delta;
  }
  return res;
}

void IPlane::on_logical_resolution_changed(
    gfx::e_resolution const resolution ) {
  on_logical_resolution_selected(
      on_logical_resolution_changed_impl( *this, resolution ) );
}

bool IPlane::supports_resolution( gfx::e_resolution ) const {
  return true;
}

} // namespace rn
