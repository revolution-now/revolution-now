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
#include "sons-of-liberty.hpp"
#include "unit-classes.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/colony.rds.hpp"
#include "ss/natives.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
FrozenDwelling dwelling_to_frozen_dwelling(
    SSConst const& ss, Dwelling const& dwelling ) {
  // This is to catch if we add any fields to ensure that we up-
  // date the below.
  static_assert(
      tuple_size_v<
          decltype( refl::traits<FrozenDwelling>::fields )> ==
      3 );
  static_assert(
      tuple_size_v<
          decltype( refl::traits<FrozenMission>::fields )> ==
      2 );

  if( dwelling.id == DwellingId{} ) {
    UNWRAP_CHECK( frozen, dwelling.frozen );
    return frozen;
  }

  Coord const location = ss.natives.coord_for( dwelling.id );
  maybe<FrozenMission> mission;
  maybe<UnitId> const  missionary_id =
      ss.units.missionary_from_dwelling( dwelling.id );
  if( missionary_id.has_value() ) {
    Unit const& unit = ss.units.unit_for( *missionary_id );
    UNWRAP_CHECK( level, missionary_type( unit.type() ) );
    mission = FrozenMission{
      .nation = unit.nation(),
      .level  = level,
    };
  }
  return FrozenDwelling{
    .tribe    = ss.natives.tribe_for( dwelling.id ).type,
    .location = location,
    .mission  = mission };
}

FrozenColony colony_to_frozen_colony( SSConst const& ss,
                                      Colony const&  colony ) {
  // This is to catch if we add any fields to ensure that we up-
  // date the below.
  static_assert(
      tuple_size_v<
          decltype( refl::traits<FrozenColony>::fields )> == 1 );

  if( colony.id == ColonyId{} ) {
    UNWRAP_CHECK( frozen, colony.frozen );
    return frozen;
  }

  int const     population = colony_population( colony );
  Player const& player =
      player_for_nation_or_die( ss.players, colony.nation );
  // In a normal game a colony population should never be zero,
  // but it is convenient to allow for this case for unit tests.
  int const sol_int_percent =
      ( population > 0 )
          ? compute_colony_sons_of_liberty( player, colony )
                .sol_integral_percent
          : 0;
  return FrozenColony{ .sons_of_liberty_integral_percent =
                           sol_int_percent };
}

void copy_real_square_to_frozen_square(
    SSConst const& ss, Coord tile,
    FrozenSquare& frozen_square ) {
  // This is to catch if we add any fields to ensure that we up-
  // date the below.
  static_assert(
      tuple_size_v<
          decltype( refl::traits<FrozenSquare>::fields )> == 3 );

  // MapSquare.
  frozen_square.square = ss.terrain.square_at( tile );

  // Colony.
  frozen_square.colony = nothing;
  if( maybe<ColonyId> const colony_id =
          ss.colonies.maybe_from_coord( tile );
      colony_id.has_value() ) {
    Colony const& colony = ss.colonies.colony_for( *colony_id );
    frozen_square.colony = colony;
    frozen_square.colony->id = 0;
    frozen_square.colony->frozen =
        colony_to_frozen_colony( ss, colony );
  }

  // Dwelling.
  frozen_square.dwelling = nothing;
  if( maybe<DwellingId> const dwelling_id =
          ss.natives.maybe_dwelling_from_coord( tile );
      dwelling_id.has_value() ) {
    Dwelling const& dwelling =
        ss.natives.dwelling_for( *dwelling_id );
    frozen_square.dwelling     = dwelling;
    frozen_square.dwelling->id = 0;
    frozen_square.dwelling->frozen =
        dwelling_to_frozen_dwelling( ss, dwelling );
  }
}

} // namespace rn
