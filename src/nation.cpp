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
#include "lua.hpp"
#include "ustate.hpp"

// game-state
#include "gs/colonies.hpp"
#include "gs/units.hpp"

// config
#include "config/nation.rds.hpp"

// luapp
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

Nationality const& nation_obj( e_nation nation ) {
  return config_nation.nations[nation];
}

maybe<e_nation> nation_from_coord(
    UnitsState const&    units_state,
    ColoniesState const& colonies_state, Coord coord ) {
  if( auto maybe_colony_id =
          colonies_state.maybe_from_coord( coord );
      maybe_colony_id )
    return colonies_state.colony_for( *maybe_colony_id )
        .nation();

  unordered_set<UnitId> const& units =
      units_state.from_coord( coord );
  if( units.empty() ) return nothing;
  e_nation first =
      units_state.unit_for( *units.begin() ).nation();
  for( auto const& id : units ) {
    (void)id; // for release builds.
    DCHECK( first == units_state.unit_for( id ).nation() );
  }
  return first;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
LUA_ENUM( nation );

} // namespace rn
