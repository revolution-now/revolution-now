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

base::valid_or<string> config::colony::warehouses::validate()
    const {
  REFL_VALIDATE(
      warehouse_max_quantity > default_max_quantity,
      "The warehouse capacity must be larger than the default "
      "capacity." );
  REFL_VALIDATE(
      warehouse_expansion_max_quantity > warehouse_max_quantity,
      "The warehouse expansion capacity must be larger than the "
      "warehouse capacity." );
  return base::valid;
}

base::valid_or<string> config_colony_t::validate() const {
  // The capenter's shop must not cost any hammers to build,
  // since one would not be able to produce the hammers to build
  // it without the building itself.
  REFL_VALIDATE(
      requirements_for_building
              [e_colony_building::carpenters_shop]
                  .hammers == 0,
      "The carpenter's shop must cost no hammers to build." );

  // Similarly, it should not have any prerequisites, since those
  // would not be buildable.
  REFL_VALIDATE( requirements_for_building
                         [e_colony_building::carpenters_shop]
                             .required_building == base::nothing,
                 "The carpenter's shop must not require any "
                 "prerequisites to build." );

  return base::valid;
}

base::valid_or<string>
config::colony::construction_requirements::validate() const {
  REFL_VALIDATE( minimum_population > 0,
                 "The minimum-required popoulation for a "
                 "construction item must be > 0." );

  return base::valid;
}

} // namespace rn
