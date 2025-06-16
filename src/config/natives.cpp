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

// refl
#include "refl/ext.hpp"

using namespace std;

namespace rn {

namespace {

// Need to pass in equipment here because this needs to be used
// by the validation code which cannot access it via the global
// config_natives object because that hasn't been populated yet.
maybe<e_native_unit_type> find_brave_impl( auto const& equipment,
                                           bool muskets,
                                           bool horses ) {
  for( auto const& [brave, eq] : equipment )
    if( eq[e_brave_equipment::muskets] == muskets &&
        eq[e_brave_equipment::horses] == horses )
      return brave;
  return nothing;
}

} // namespace

/****************************************************************
** Validation.
*****************************************************************/
base::valid_or<string> config::natives::ColonyAttack::validate()
    const {
  int total = 0;
  for( auto outcome :
       refl::enum_values<e_brave_attack_colony_effect> )
    total += outcome_weights[outcome];
  REFL_VALIDATE( total == 100,
                 "Brave colony attack outcome weights must sum "
                 "to 100, but instead sum to {}.",
                 total );

  REFL_VALIDATE( commodity_percent_stolen.min > 0.0,
                 "commodity_percent_stolen.min must be > 0.0." );
  REFL_VALIDATE( commodity_percent_stolen.min < 1.0,
                 "commodity_percent_stolen.min must be < 1.0." );
  REFL_VALIDATE( commodity_percent_stolen.max > 0.0,
                 "commodity_percent_stolen.max must be > 0.0." );
  REFL_VALIDATE( commodity_percent_stolen.max < 1.0,
                 "commodity_percent_stolen.max must be < 1.0." );
  REFL_VALIDATE( commodity_percent_stolen.min <=
                     commodity_percent_stolen.max,
                 "commodity_percent_stolen.min must be <= "
                 "commodity_percent_stolen.max" );

  return base::valid;
}

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

  REFL_VALIDATE(
      growth_counter_threshold > 0,
      "growth_counter_threshold must be larger than 0." );

  return base::valid;
}

base::valid_or<string> config::natives::Arms::validate() const {
  REFL_VALIDATE( internal_muskets_per_armed_brave > 0,
                 "internal_muskets_per_armed_brave must be "
                 "larger than 0." );

  REFL_VALIDATE( internal_horses_per_mounted_brave > 0,
                 "internal_horses_per_mounted_brave must be "
                 "larger than 0." );

  for( bool const muskets : { false, true } ) {
    for( bool const horses : { false, true } ) {
      REFL_VALIDATE(
          find_brave_impl( equipment, muskets, horses )
              .has_value(),
          "cannot find a brave with muskets={} and horses={}.",
          muskets, horses );
    }
  }

  // We're not allowed to change these otherwise the logic that
  // tries to promote braves in combat will fail.
  {
    using enum e_native_unit_type;
    using enum e_brave_equipment;
    string const no_change_equipment =
        "changing brave equipment config values is not allowed.";
    REFL_VALIDATE( equipment[brave][muskets] == false, "{}",
                   no_change_equipment );
    REFL_VALIDATE( equipment[brave][horses] == false, "{}",
                   no_change_equipment );
    REFL_VALIDATE( equipment[armed_brave][muskets] == true, "{}",
                   no_change_equipment );
    REFL_VALIDATE( equipment[armed_brave][horses] == false, "{}",
                   no_change_equipment );
    REFL_VALIDATE( equipment[mounted_brave][muskets] == false,
                   "{}", no_change_equipment );
    REFL_VALIDATE( equipment[mounted_brave][horses] == true,
                   "{}", no_change_equipment );
    REFL_VALIDATE( equipment[mounted_warrior][muskets] == true,
                   "{}", no_change_equipment );
    REFL_VALIDATE( equipment[mounted_warrior][horses] == true,
                   "{}", no_change_equipment );
  }
  return base::valid;
}

/****************************************************************
** Public API.
*****************************************************************/
e_native_unit_type find_brave( bool muskets, bool horses ) {
  UNWRAP_CHECK_MSG(
      type,
      find_brave_impl( config_natives.arms.equipment, muskets,
                       horses ),
      "internal error: cannot find brave for muskets={} and "
      "horses={}, but config validation should have ensured "
      "that it exists.",
      muskets, horses );
  return type;
}

} // namespace rn
