/****************************************************************
**colony.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-06.
*
* Description: Config info for colonies (non-production).
*
*****************************************************************/
#include "colony.hpp"

using namespace std;

namespace rn {

base::valid_or<string> Warehouses::validate() const {
  REFL_VALIDATE(
      warehouse_max_quantity > default_max_quantity,
      "The warehouse capacity must be larger than the default "
      "capacity." );
  REFL_VALIDATE(
      warehouse_expansion_max_quantity > warehouse_max_quantity,
      "The warehouse expansion capacity must be larger than the "
      "warehouse capacity." );
  REFL_VALIDATE(
      food_max_quantity >= default_max_quantity,
      "max food capacity must be larger than default warehouse "
      "capacity." );
  return base::valid;
}

base::valid_or<string> config_colony_t::validate() const {
  REFL_VALIDATE(
      food_for_creating_new_colonist <=
          warehouses.food_max_quantity,
      "The amount of food required for creating a new colonist "
      "must be <= to the maximum quantity of food allowed." );
  return base::valid;
}

} // namespace rn
