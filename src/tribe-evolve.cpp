/****************************************************************
**tribe-evolve.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-24.
*
* Description: Evolves a tribe's dwelling by one turn.
*
*****************************************************************/
#include "tribe-evolve.hpp"

// config
#include "config/natives.rds.hpp"

// Revoution Now
#include "logger.hpp"
#include "unit-mgr.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

void evolve_dwelling( SS& ss, Dwelling& dwelling ) {
  e_tribe const tribe_type =
      ss.natives.tribe_type_for( dwelling.id );

  // Evolve population.
  int const max_population =
      dwelling.is_capital
          ? config_natives.tribes[tribe_type]
                .max_population_capital
          : config_natives.tribes[tribe_type].max_population;
  bool const has_brave =
      !ss.units.braves_for_dwelling( dwelling.id ).empty();
  bool const has_max_population =
      ( dwelling.population >= max_population );
  if( has_brave && has_max_population )
    dwelling.growth_counter = 0;
  else
    dwelling.growth_counter += dwelling.population;

  if( dwelling.growth_counter >=
      config_natives.growth_counter_threshold ) {
    dwelling.growth_counter = 0;
    if( !has_brave ) {
      lg.debug( "created new brave for dwelling {}.",
                dwelling.id );
      create_unit_on_map_non_interactive(
          ss, e_native_unit_type::brave,
          ss.natives.coord_for( dwelling.id ), dwelling.id );
    } else {
      CHECK( !has_max_population,
             "expected dwelling {}'s population to be less than "
             "the max of {} but instead it is {}.",
             dwelling.id, max_population, dwelling.population );
      ++dwelling.population;
      lg.debug( "increased population for dwelling {}.",
                dwelling.id );
    }
  }
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void evolve_dwellings_for_tribe( SS& ss, e_tribe tribe_type ) {
  unordered_set<DwellingId> const& dwellings =
      ss.natives.dwellings_for_tribe( tribe_type );
  for( DwellingId const dwelling_id : dwellings )
    evolve_dwelling( ss,
                     ss.natives.dwelling_for( dwelling_id ) );
}

} // namespace rn
