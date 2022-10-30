/****************************************************************
**nation.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-03.
*
* Description: Representation of nations.
*
*****************************************************************/
#include "nation.hpp"

// Revolution Now
#include "colony.hpp"
#include "ustate.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/units.hpp"

// config
#include "config/nation.rds.hpp"

// luapp
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

maybe<e_nation> nation_from_coord(
    UnitsState const&    units_state,
    ColoniesState const& colonies_state, Coord coord ) {
  if( auto maybe_colony_id =
          colonies_state.maybe_from_coord( coord );
      maybe_colony_id )
    return colonies_state.colony_for( *maybe_colony_id ).nation;

  unordered_set<GenericUnitId> const& units =
      units_state.from_coord( coord );
  if( units.empty() ) return nothing;
  GenericUnitId const first = *units.begin();
  if( units_state.unit_kind( first ) != e_unit_kind::euro )
    return nothing;
  return units_state.euro_unit_for( first ).nation();
}

} // namespace rn
