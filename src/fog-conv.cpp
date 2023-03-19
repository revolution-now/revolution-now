/****************************************************************
**fog-conv.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-18.
*
* Description: Converts real entities into fogged entities.
*
*****************************************************************/
#include "fog-conv.hpp"

// Revolution Now
#include "colony-buildings.hpp"
#include "colony-mgr.hpp"
#include "unit-classes.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/colony.rds.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
FogDwelling dwelling_to_fog_dwelling( SSConst const& ss,
                                      DwellingId dwelling_id ) {
  // This is to catch if we add any fields to ensure that we up-
  // date the below.
  static_assert(
      tuple_size_v<
          decltype( refl::traits<FogDwelling>::fields )> == 3 );
  static_assert(
      tuple_size_v<
          decltype( refl::traits<FogMission>::fields )> == 2 );

  Dwelling const& dwelling =
      ss.natives.dwelling_for( dwelling_id );
  maybe<FogMission>   mission;
  maybe<UnitId> const missionary_id =
      ss.units.missionary_from_dwelling( dwelling_id );
  if( missionary_id.has_value() ) {
    Unit const& unit = ss.units.unit_for( *missionary_id );
    UNWRAP_CHECK( level, missionary_type( unit.type() ) );
    mission = FogMission{
        .nation = unit.nation(),
        .level  = level,
    };
  }
  return FogDwelling{
      .tribe   = ss.natives.tribe_for( dwelling_id ).type,
      .capital = dwelling.is_capital,
      .mission = mission };
}

FogColony colony_to_fog_colony( Colony const& colony ) {
  // This is to catch if we add any fields to ensure that we up-
  // date the below.
  static_assert(
      tuple_size_v<
          decltype( refl::traits<FogColony>::fields )> == 4 );
  return FogColony{
      .nation         = colony.nation,
      .name           = colony.name,
      .population     = colony_population( colony ),
      .barricade_type = barricade_for_colony( colony ) };
}

void copy_real_square_to_fog_square( SSConst const& ss,
                                     Coord          tile,
                                     FogSquare& fog_square ) {
  // This is to catch if we add any fields to ensure that we up-
  // date the below.
  static_assert(
      tuple_size_v<
          decltype( refl::traits<FogSquare>::fields )> == 4 );

  // MapSquare.
  fog_square.square = ss.terrain.square_at( tile );

  // Colony.
  fog_square.colony = nothing;
  if( maybe<ColonyId> const colony_id =
          ss.colonies.maybe_from_coord( tile );
      colony_id.has_value() )
    fog_square.colony = colony_to_fog_colony(
        ss.colonies.colony_for( *colony_id ) );

  // Dwelling.
  fog_square.dwelling = nothing;
  if( maybe<DwellingId> const dwelling_id =
          ss.natives.maybe_dwelling_from_coord( tile );
      dwelling_id.has_value() )
    fog_square.dwelling =
        dwelling_to_fog_dwelling( ss, *dwelling_id );
}

} // namespace rn
