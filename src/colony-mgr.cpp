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

// base-util
#include "base-util/string.hpp"

// magic_enum
#include "magic_enum.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
expect<> check_colony_invariants_safe( ColonyId id ) {
  // 0.  Colony is reported as existing.
  if( !colony_exists( id ) )
    return UNEXPECTED( "Colony does not exist." );

  auto const& colony = colony_from_id( id );

  // 1.  Colony location matches coord.
  auto coord = colony.location();
  if( colony_from_coord( coord ) != id )
    return UNEXPECTED( "Inconsistent colony map coordinate." );

  // 2.  Colony is on land.
  if( !terrain_is_land( coord ) )
    return UNEXPECTED( "Colony is not on land." );

  // 3.  Colony has non-empty stripped name.
  if( util::strip( colony.name() ).empty() )
    return UNEXPECTED( "Colony name is empty (when stripped)." );

  // 4.  Colony has at least one colonist.
  if( colony.units_jobs().size() == 0 )
    return UNEXPECTED( "Colony {} has no units.", colony );

  // 5.  All colony's units owned by colony.
  for( auto const& p : colony.units_jobs() ) {
    auto unit_id = p.first;
    if( state_for_unit( unit_id ) != e_unit_state::colony )
      return UNEXPECTED(
          "Unit {} in Colony {} is not owned by colony.",
          debug_string( unit_id ), colony );
  }

  // 6.  All colony's units can occupy a colony.
  // TODO: this requires developing logic to classify units and
  // detect when they are carrying items that could be discarded
  // to put them in a colony (e.g. a soldier discarding muskets
  // to become a free colonist).

  // 7.  All colony's units are of same nation.
  for( auto const& p : colony.units_jobs() ) {
    auto nation = unit_from_id( p.first ).nation();
    if( colony.nation() != unit_from_id( p.first ).nation() )
      return UNEXPECTED(
          "Colony has nation {} but unit has nation {}.",
          colony.nation(), nation );
  }

  // 8.  Colony's commodity quantites in correct range.
  for( auto [comm, quantity] : colony.commodities() ) {
    if( quantity < 0 )
      return UNEXPECTED( "Colony has negative quantity of {}.",
                         comm );
  }

  // 9.  Colony's building set is self-consistent.
  // TODO

  // 10. Unit mfg jobs are self-consistent.
  // TODO

  // 11. Unit land jobs are good.
  // TODO

  // 12. Colony's production status is self-consistent.
  // TODO

  return xp_success_t();
}

void check_colony_invariants_die( ColonyId id ) {
  CHECK_XP( check_colony_invariants_safe( id ) );
}

e_found_colony_result can_found_colony( UnitId founder ) {
  using Res_t = e_found_colony_result;
  auto& unit  = unit_from_id( founder );

  if( unit.desc().ship ) return Res_t::ship_cannot_found_colony;

  auto maybe_coord = coord_for_unit_indirect_safe( founder );
  if( !maybe_coord.has_value() )
    return Res_t::colonist_not_on_map;

  if( colony_from_coord( *maybe_coord ) )
    return Res_t::colony_exists_here;

  if( !terrain_is_land( *maybe_coord ) )
    return Res_t::no_water_colony;

  return Res_t::good;
}

expect<ColonyId> found_colony( UnitId           founder,
                               std::string_view name ) {
  if( auto res = can_found_colony( founder );
      res != e_found_colony_result::good )
    return UNEXPECTED( "Cannot found colony, error code: {}.",
                       magic_enum::enum_name( res ) );

  auto nation = unit_from_id( founder ).nation();
  ASSIGN_CHECK_OPT( where,
                    coord_for_unit_indirect_safe( founder ) );

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

LUA_FN( found_colony, ColonyId, UnitId founder,
        std::string const& name ) {
  ASSIGN_CHECK_XP( id, found_colony( founder, name ) );
  return id;
}

} // namespace

} // namespace rn
