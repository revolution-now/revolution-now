/****************************************************************
* unit.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-28.
*
* Description: Data structure for units.
*
*****************************************************************/

#include "unit.hpp"
#include "world.hpp"

#include <unordered_map>
#include <unordered_set>

using namespace std;

namespace rn {

namespace {

unordered_map<int, Unit> units;

unordered_map<Coord, unordered_set<int>> units_from_coords;
  
} // namespace

} // namespace rn

