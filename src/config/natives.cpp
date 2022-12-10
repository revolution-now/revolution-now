/****************************************************************
**natives.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-22.
*
* Description: Config info for the natives.
*
*****************************************************************/
#include "natives.hpp"

// config
#include "natives.hpp"

// refl
#include "refl/ext.hpp"

using namespace std;

namespace rn {

base::valid_or<string> config::natives::LandPrices::validate()
    const {
  REFL_VALIDATE( bonus_prime_resource >= 1.0,
                 "bonus_prime_resource must be >= 1.0." );
  REFL_VALIDATE( bonus_capital >= 1.0,
                 "bonus_capital must be >= 1.0." );
  REFL_VALIDATE( tribal_anger_max_n >= 0,
                 "tribal_anger_max_n must be >= 0." );
  REFL_VALIDATE( tribal_anger_max_n <= 100,
                 "tribal_anger_max_n must be <= 100." );
  REFL_VALIDATE( distance_exponential <= 1.0,
                 "distance_exponential must be <= 1.0." );
  return base::valid;
}

base::valid_or<string> config::natives::AlarmLandGrab::validate()
    const {
  REFL_VALIDATE( prime_resource_scale >= 1.0,
                 "prime_resource_scale must be >= 1.0." );
  REFL_VALIDATE( distance_factor <= 1.0,
                 "distance_factor must be <= 1.0." );
  return base::valid;
}

} // namespace rn
