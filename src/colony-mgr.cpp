/****************************************************************
**colony-mgr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-01.
*
* Description: Main interface for controlling Colonies.
*
*****************************************************************/
#include "colony-mgr.hpp"

// Revolution Now
#include "colony.hpp"
#include "cstate.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "terrain.hpp"
#include "ustate.hpp"

// magic_enum
#include "magic_enum.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
expect<> check_colony_invariants_safe( ColonyId id ) {
  // TODO
  (void)id;
  return xp_success_t();
}

void check_colony_invariants_die( ColonyId id ) {
  CHECK_XP( check_colony_invariants_safe( id ) );
}

e_found_colony_result can_found_colony( UnitId founder,
                                        Coord  where ) {
  if( auto coord = coord_for_unit_indirect_safe( founder );
      !coord )
    return e_found_colony_result::colonist_not_on_land;
  else if( *coord != where )
    return e_found_colony_result::colonist_not_at_site;

  if( colony_from_coord( where ) )
    return e_found_colony_result::colony_exists_here;

  if( !terrain_is_land( where ) )
    return e_found_colony_result::no_water_colony;

  return e_found_colony_result::good;
}

expect<ColonyId> found_colony( UnitId founder, Coord where,
                               std::string_view name ) {
  if( auto res = can_found_colony( founder, where );
      res != e_found_colony_result::good )
    return UNEXPECTED( "Cannot found colony, error code: {}.",
                       magic_enum::enum_name( res ) );

  auto nation = unit_from_id( founder ).nation();

  // 1. Create colony object.
  XP_OR_RETURN( col_id,
                cstate_create_colony( nation, where, name ) );

  // 2. Find initial job for founder. (TODO)
  ColonyJob_t job =
      ColonyJob::mfg{ .mfg_job = e_mfg_job::bells };

  // 3. Move unit into it.
  //
  // TODO: if unit is carrying horses, tools, or muskets, those
  // need to be stripped and added into the colony's commodities.
  ustate_change_to_colony( founder, col_id, job );

  // 4. Check colony invariants. Need to abort here if the in-
  // variants are violated because we have already created the
  // colony, and so this should never happen.
  CHECK_XP( check_colony_invariants_safe( col_id ) );

  // 5. Done.
  auto& desc = nation_obj( nation );
  lg.info( "created {} {} colony at {}.", desc.article,
           desc.adjective, where );
  return col_id;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( found_colony, ColonyId, UnitId founder, Coord where,
        std::string const& name ) {
  ASSIGN_CHECK_XP( id, found_colony( founder, where, name ) );
  return id;
}

} // namespace

} // namespace rn
