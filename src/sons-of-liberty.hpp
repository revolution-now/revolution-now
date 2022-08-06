/****************************************************************
**sons-of-liberty.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-19.
*
* Description: Logic related to Sons of Liberty.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "sons-of-liberty.rds.hpp"

// ss
#include "ss/difficulty.rds.hpp"

namespace rn {

struct Colony;
struct Player;
struct SettingsState;

// This will be the true sons of liberty membership percent of
// the colony. NOTE: this is not what is precisely displayed in
// the UI; that displayed version will be rounded, and is com-
// puted instead from a function below, and it is that (integral)
// value that should be used to compute things that depend on the
// SoL % in order that things agree with what the player sees.
double compute_sons_of_liberty_percent(
    double num_rebels_from_bells_only, int colony_population,
    bool has_simon_bolivar );

// This is the SoL percent that will be displayed in the UI,
// which is always an integral number in [0, 100] since it looks
// nicer than adding decimal places. Therefore, in order for
// things to be consistent with what the player is seeing, anny-
// thing that is computed based on SoL percent that is then vis-
// ible to the player should be computed based on this number,
// e.g. production bonuses, tory penalties, notifications, etc.
int compute_sons_of_liberty_integral_percent(
    double sons_of_liberty_percent );

// This is the (integral) number of colonists in the colony that
// are members of the Sons of Liberty. This is what is displayed
// in the UI. Note that the sons_of_liberty_integral_percent must
// be computed using the above function, and we use the integral
// percent instead of the floating percent (which is more accu-
// rate) because the (rounded) integral percent is what's dis-
// played in the UI, and we want this number to always be consis-
// tent with it, and the rounding logic is non-trivial.
int compute_sons_of_liberty_number(
    int sons_of_liberty_integral_percent,
    int colony_population );

// This is the (integral) number of colonists in the colony that
// are tories (loyal to the crown). This is what is displayed in
// the UI.
int compute_tory_number( int sons_of_liberty_number,
                         int colony_population );

// This will compute the production bonus to add as a result of
// the sons of liberty percent. We want to use the integral per-
// cent because that is what will be displayed to the user in the
// UI, and we won't have to worry about floating point rounding
// errors below. This way, we easily guarantee that the bonus is
// always consistent with what the player sees in the UI.
int compute_sons_of_liberty_bonus_indoor(
    int sons_of_liberty_integral_percent );
int compute_sons_of_liberty_bonus_outdoor(
    int sons_of_liberty_integral_percent );

// How many tory penalty levels is this colony being penalized
// with. E.g. on Viceroy, 5 rebels will be at level 0, meaning no
// penalty; 6 will be at level 1, 12 at level 2, etc.
int compute_tory_penalty_level( e_difficulty difficulty,
                                int          tory_number );

// This will compute the production penalty to subtract as a re-
// sult of the tory penalty.
int compute_tory_penalty_indoor( e_difficulty difficulty,
                                 int          tory_number );
int compute_tory_penalty_outdoor( e_difficulty difficulty,
                                  int          tory_number );

// This will take the (fractional) number of rebels in a colony
// as computed without the Bolivar bonus and will evolve it one
// turn given the bell production and current population. It is
// ok if the num_rebels_from_bells_only is larger than the popu-
// lation; this can happen if the colony population was reduced
// since this was last computed and in that case it will just be
// truncated to the colony population before proceeding with the
// calculation.
double evolve_num_rebels_from_bells_only(
    double num_rebels_from_bells_only, int bells_produced,
    int colony_population );

ColonySonsOfLiberty compute_colony_sons_of_liberty(
    Player const& player, Colony const& colony );

} // namespace rn
