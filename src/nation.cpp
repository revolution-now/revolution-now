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
#include "cstate.hpp"
#include "lua.hpp"
#include "ustate.hpp"

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

maybe<e_nation> nation_from_coord( Coord coord ) {
  if( auto maybe_colony_id = colony_from_coord( coord );
      maybe_colony_id )
    return colony_from_id( *maybe_colony_id ).nation();

  auto const& units = units_from_coord( coord );
  if( units.empty() ) return nothing;
  e_nation first = unit_from_id( *units.begin() ).nation();
  for( auto const& id : units ) {
    (void)id; // for release builds.
    DCHECK( first == unit_from_id( id ).nation() );
  }
  return first;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
LUA_ENUM( nation );

} // namespace rn
