/****************************************************************
**land-square.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-02-01.
*
* Description: Represents a single square of land.
*
*****************************************************************/
#include "land-square.hpp"

using namespace std;

namespace rn {

valid_deserial_t LandSquare::check_invariants_safe() const {
  return valid;
}

} // namespace rn
