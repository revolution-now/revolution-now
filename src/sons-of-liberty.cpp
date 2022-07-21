/****************************************************************
**sons-of-liberty.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-19.
*
* Description: Logic related to Sons of Liberty.
*
*****************************************************************/
#include "sons-of-liberty.hpp"

// config
#include "config/colony.rds.hpp"

// base
#include "base/error.hpp"

// C++ standard library
#include <algorithm>

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
// This will be the true sons of liberty membership percent of
// the colony.  However, this is not what is precisely displayed
// in the UI; that displayed version will be rounded.
double compute_sons_of_liberty_percent(
    double num_rebels_from_bells_only, int colony_population,
    bool has_simon_bolivar ) {
  CHECK( colony_population > 0,
         "A colony's population must be > 0 in order to compute "
         "rebel %." );
  CHECK_GE( num_rebels_from_bells_only, 0.0 );
  // The num_rebels_from_bells_only could be larger than the pop-
  // ulation if colonists were removed from the colony after it
  // was computed. If so, it will be truncated at the start of
  // the next turn when it is recomputed. But for now we can just
  // truncate it without losing anything.
  double const pre_bolivar = std::min(
      num_rebels_from_bells_only / colony_population, 1.0 );
  // Bolivar bonus, since it is additive, can put the percent
  // above 100%. In that case we will just cap it.
  double const post_bolivar = std::min(
      has_simon_bolivar
          ? ( pre_bolivar +
              config_colony.bolivar_sons_of_liberty_bonus )
          : pre_bolivar,
      1.0 );

  CHECK_GE( post_bolivar, 0.0 );
  CHECK_LE( post_bolivar, 1.0 );
  return post_bolivar;
}

int compute_sons_of_liberty_number(
    double sons_of_liberty_percent, int colony_population ) {
  int const rounded_colonists = static_cast<int>( std::floor(
      sons_of_liberty_percent * colony_population ) );
  CHECK_GE( rounded_colonists, 0 );
  CHECK_LE( rounded_colonists, colony_population );
  return rounded_colonists;
}

int compute_tory_number( int sons_of_liberty_number,
                         int colony_population ) {
  CHECK_GE( sons_of_liberty_number, 0 );
  CHECK_LE( sons_of_liberty_number, colony_population );
  return colony_population - sons_of_liberty_number;
}

double evolve_num_rebels_from_bells_only(
    double num_rebels_from_bells_only, int bells_produced,
    int colony_population ) {
  // This could be larger than the population if the colony popu-
  // lation was reduced since it was last computed. In that case
  // we want to truncate it first before proceeding with the cal-
  // culation since the evolution depends on the current value.
  // Note that we also need to perform a similar clamp at the
  // end, since the final result can be larger than the popula-
  // tion if the bell production is high enough.
  num_rebels_from_bells_only =
      std::clamp( num_rebels_from_bells_only, 0.0,
                  double( colony_population ) );
  // The idea here is that:
  //
  //   1. Rebels (which can be fractional) consume two bells per
  //      turn per rebel for "maintenance." Any bells that remain
  //      are considered surplus. If there is a shortage of bells
  //      then the fractional rebel number will decrease.
  //   2. It takes an accumulation of 100 surplus bells to con-
  //      vert one tory full to SoL membership.
  //
  // Despite #2, we don't actually need to maintain the number of
  // accumulated bells in order to determine when the 100 surplus
  // bells have been reached. Instead, we will do what the orig-
  // inal game appears to do, which is to evolve the fractional
  // rebels number (denoted R) with a differential equation that
  // evolves it directly without needing to know the bell accumu-
  // lation, and this should be mathematically equivalent (to the
  // alternative method of maintaining nominal accumulated bell
  // counts).
  //
  // If R denotes the number of fractional rebels, and B denotes
  // the rate of bell production, the below differential equation
  // implements the desired evolution:
  //
  //   delta R(t)    B - 2R(t)
  //   ---------- = ----------
  //    delta t        100
  //
  // This is a first order differential equation in time with a
  // negative coefficient in front of the R, meaning that it
  // tends toward an equilibrium position R_eq:
  //
  //           B
  //   R_eq = ---
  //           2
  //
  // i.e., if a colony has a fixed bell production rate of 1 bell
  // per turn then R will tend toward a value of 1/2. Further-
  // more, if the colony has a population P, then the pre-bolivar
  // Sons of Liberty membership percent will tend toward (S_eq):
  //
  //          R_eq   .5
  //   S_eq = ---- = --- = .25
  //           P      2
  //
  // It is claimed by this author that these are the formulas
  // used by the original game. However, in testing it was noted
  // that some times the numbers can disagree with these formu-
  // las; for example, multiple people (including this author)
  // have measured that sometimes a colony with population 1 and
  // 1 bell production will only reach a maximum SoL percent of
  // .33 (as opposed to the .5 predicted by the above). Further-
  // more, in some cases the original game seems to exhibit two
  // equilibrium points for a given bell production/population,
  // and a stable zone between them where no time evolution oc-
  // curs.
  //
  // Some experimentation reveals that this is likely due to the
  // following non-idealities:
  //
  //   1. The equation being simulated is actually a difference
  //      equation and not a differential equation, because each
  //      time we iterate it, the delta_t is one turn, as opposed
  //      to an "infinitesimal" value.
  //   2. The original game likely has some non-ideal rounding
  //      behavior and/or floating point handling.
  //
  // Simulations done of iterating over this difference equation
  // while doing course-grained rounding at each step were able
  // to successfully reproduce the strange behavior described
  // above. However, in our simulations, although we will be
  // using the same difference equation, we will be using more
  // precise floating point processing with only negligible
  // rounding errors, and so we will not see those strange ef-
  // fects as in the original game.
  //
  // Therefore, in some cases our numbers (and equilibrium
  // points) will differ from the original game (sometimes no-
  // ticeably), but it seems like a good improvement.
  double const delta = ( double( bells_produced ) -
                         config_colony.bells_consumed_per_rebel *
                             num_rebels_from_bells_only ) /
                       100.0;
  return std::clamp( num_rebels_from_bells_only + delta, 0.0,
                     double( colony_population ) );
}

int compute_sons_of_liberty_integral_percent(
    double sons_of_liberty_percent ) {
  // Note that we have this special case where we round anything
  // over 99% to 100% because otherwise the asymptotic evolution
  // of the SoL to 100 will never actually reach it, whereas in
  // the original game it is observed that even when you have
  // just the right number of bells each turn to make a given
  // colony able to asymptotically reach 100, it will eventually
  // do so. It may be the case that in the original game that be-
  // havior is due to rounding errors, and so our asymptotic be-
  // havior won't be precisely the same (it appears to move more
  // slowly near the limit), but at least this way it will even-
  // tually make it hit the target after enough turns. This is to
  // provide a slightly better player experience.
  if( sons_of_liberty_percent > .99 ) return 100;
  int const res = static_cast<int>(
      std::lround( sons_of_liberty_percent * 100.0 ) );
  CHECK_GE( res, 0 );
  CHECK_LE( res, 100 );
  return res;
}

int compute_sons_of_liberty_bonus(
    int sons_of_liberty_integral_percent ) {
  CHECK_GE( sons_of_liberty_integral_percent, 0 );
  CHECK_LE( sons_of_liberty_integral_percent, 100 );
  if( sons_of_liberty_integral_percent == 100 )
    return config_colony.sons_of_liberty_100_bonus;
  if( sons_of_liberty_integral_percent >= 50 )
    return config_colony.sons_of_liberty_50_bonus;
  return 0;
}

int compute_tory_penalty( e_difficulty difficulty,
                          int          tory_number ) {
  int const penalty_population =
      config_colony.tory_penalty_population[difficulty];
  // For each multiple of the penalty population that the colony
  // has that are tories, we produce one penalty point. Each
  // point will be subtracted from the production of all
  // colonists in the colony.
  return config_colony.tory_production_penalty *
         ( tory_number / penalty_population );
}

} // namespace rn
