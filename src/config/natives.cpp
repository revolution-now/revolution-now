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

base::valid_or<string>
config::natives::SpeakWithChief::validate() const {
  REFL_VALIDATE( alarm_range_for_target_practice.min >= 0,
                 "alarm_range_for_target_practice.min "
                 "must be >= 0." );
  REFL_VALIDATE( alarm_range_for_target_practice.min <= 99,
                 "alarm_range_for_target_practice.min "
                 "must be <= 99." );
  REFL_VALIDATE( alarm_range_for_target_practice.max >= 0,
                 "alarm_range_for_target_practice.max "
                 "must be >= 0." );
  REFL_VALIDATE( alarm_range_for_target_practice.max <= 99,
                 "alarm_range_for_target_practice.max "
                 "must be <= 99." );
  REFL_VALIDATE(
      alarm_range_for_target_practice.max >=
          alarm_range_for_target_practice.min,
      "alarm_range_for_target_practice.max must be >= "
      "than alarm_range_for_target_practice.min" );
  REFL_VALIDATE(
      positive_outcome_weights[e_speak_with_chief_result::none] >
          0,
      "weight for positive_outcome_weights.none must be > 0." );
  return base::valid;
}

base::valid_or<string> config_natives_t::validate() const {
  REFL_VALIDATE(
      speak_with_chief[e_scout_type::seasoned]
              .positive_outcome_weights
                  [e_speak_with_chief_result::promotion] == 0,
      "the probability weight for a seasoned scout to get "
      "promoted while speaking to a chief must be zero since a "
      "seasoned scout cannot get promoted." );
  return base::valid;
}

} // namespace rn
