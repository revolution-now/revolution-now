/****************************************************************
**revolution.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-12.
*
* Description: Config data for the revolution module.
*
*****************************************************************/
#include "revolution.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

static_assert(
    tuple_size_v<
        decltype( refl::traits<ExpeditionaryForce>::fields )> ==
        4,
    "Probably don't want to add more to this struct since it is "
    "also used in this config module." );

}

/****************************************************************
** config::revolution::Declaration
*****************************************************************/
base::valid_or<string>
config::revolution::Declaration::validate() const {
  for( auto const& [difficulty, rebels] :
       ai_required_number_of_rebels )
    REFL_VALIDATE(
        rebels > 0,
        "Required number of rebels for AI to be granted "
        "independence must be larger than zero." );

  return base::valid;
}

/****************************************************************
** config::revolution::InterventionForces
*****************************************************************/
base::valid_or<string>
config::revolution::InterventionForces::validate() const {
  int constexpr kUnitsPerShip = 6;
  static_assert( kUnitsPerShip > 0 );
  for( auto const& [difficulty, force] : unit_counts ) {
    int const ships_needed =
        ( force.continental_army + force.continental_cavalry +
          force.artillery + kUnitsPerShip - 1 ) /
        kUnitsPerShip;
    REFL_VALIDATE( force.men_o_war >= ships_needed,
                   "Intervention force on difficulty level {} "
                   "requires at least {} ships to transport "
                   "units, but only {} provided.",
                   difficulty, ships_needed, force.men_o_war );
  }

  return base::valid;
}

} // namespace rn
