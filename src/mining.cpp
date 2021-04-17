/****************************************************************
**mining.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-25.
*
* Description: Logic for computing production on a square.
*
*****************************************************************/
#include "mining.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
UnitLandProductionInfo production_for_landsquare(
    LandSquare const& land_square, e_commodity commodity,
    e_unit_type utype ) {
  if( utype == e_unit_type::free_colonist &&
      land_square.surface == e_surface::land &&
      commodity == e_commodity::food ) {
    return UnitLandProductionInfo{
        .quantity = 4, .reason = e_land_production_reason::ok };
  }
  return UnitLandProductionInfo{
      .quantity = 0,
      .reason   = e_land_production_reason::void_of_commodity };
}

} // namespace rn
