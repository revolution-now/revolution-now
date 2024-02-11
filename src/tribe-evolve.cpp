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
#include "tribe-arms.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

void spawn_brave( SS& ss, IRand& rand, Dwelling const& dwelling,
                  Tribe& tribe ) {
  lg.debug( "created new brave for dwelling {}.", dwelling.id );
  EquippedBrave const spawned = select_new_brave_equip(
      ss.as_const, rand, as_const( tribe ) );
  tribe.muskets += spawned.muskets_delta;
  tribe.horse_breeding += spawned.horse_breeding_delta;
  CHECK_GE( tribe.muskets, 0 );
  CHECK_GE( tribe.horse_breeding, 0 );
  create_unit_on_map_non_interactive(
      ss, spawned.type, ss.natives.coord_for( dwelling.id ),
      dwelling.id );
}

void evolve_dwelling( SS& ss, TS& ts, Dwelling& dwelling ) {
  e_tribe const tribe_type =
      ss.natives.tribe_type_for( dwelling.id );
  Tribe& tribe = ss.natives.tribe_for( tribe_type );

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
      spawn_brave( ss, ts.rand, dwelling, tribe );
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
void evolve_tribe_common( SS& ss, e_tribe tribe_type ) {
  Tribe& tribe = ss.natives.tribe_for( tribe_type );

  // Horses.
  evolve_tribe_horse_breeding( ss, tribe );
}

void evolve_dwellings_for_tribe( SS& ss, TS& ts,
                                 e_tribe tribe_type ) {
  unordered_set<DwellingId> const& dwellings =
      ss.natives.dwellings_for_tribe( tribe_type );
  for( DwellingId const dwelling_id : dwellings )
    evolve_dwelling( ss, ts,
                     ss.natives.dwelling_for( dwelling_id ) );
}

} // namespace rn
