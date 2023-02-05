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

namespace {

bool contains( vector<int> const& v, int n ) {
  return find( v.begin(), v.end(), n ) != v.end();
}
}

base::valid_or<string> config::colony::Notifications::validate()
    const {
  REFL_VALIDATE(
      contains( sons_of_liberty_notification_thresholds, 100 ),
      "sons_of_liberty_notification_thresholds must contain "
      "100." );
  REFL_VALIDATE(
      contains( sons_of_liberty_notification_thresholds, 50 ),
      "sons_of_liberty_notification_thresholds must contain "
      "50." );
  return base::valid;
}

base::valid_or<string> config::colony::CustomHouse::validate()
    const {
  REFL_VALIDATE(
      threshold_for_sell >= amount_to_retain,
      "threshold_for_sell must be >= amount_to_retrain." );
  return base::valid;
}

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

base::valid_or<string>
config::colony::on_the_job_training::validate() const {
  for( auto const& [unit_type, probability] : probabilities ) {
    REFL_VALIDATE(
        probability >= 0.0 && probability <= 1.0,
        "on-the-job training probabilities must be between 0.0 "
        "and 1.0, instead found {} for unit type {}.",
        probability, unit_type );
  }
  return base::valid;
}

base::valid_or<string> config_colony_t::validate() const {
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
