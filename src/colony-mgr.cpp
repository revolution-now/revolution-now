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
#include "co-waitable.hpp"
#include "colony-view.hpp"
#include "colony.hpp"
#include "cstate.hpp"
#include "enum.hpp"
#include "fmt-helper.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "rand.hpp"
#include "terrain.hpp"
#include "ustate.hpp"
#include "window.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// base-util
#include "base-util/string.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
valid_or<generic_err> check_colony_invariants_safe(
    ColonyId id ) {
  // 0.  Colony is reported as existing.
  if( !colony_exists( id ) )
    return GENERIC_ERROR( "Colony does not exist." );

  auto const& colony = colony_from_id( id );

  // 1.  Colony location matches coord.
  auto coord = colony.location();
  if( colony_from_coord( coord ) != id )
    return GENERIC_ERROR(
        "Inconsistent colony map coordinate." );

  // 2.  Colony is on land.
  if( !terrain_is_land( coord ) )
    return GENERIC_ERROR( "Colony is not on land." );

  // 3.  Colony has non-empty stripped name.
  if( util::strip( colony.name() ).empty() )
    return GENERIC_ERROR(
        "Colony name is empty (when stripped)." );

  // 4.  Colony has at least one colonist.
  if( colony.units_jobs().size() == 0 )
    return GENERIC_ERROR( "Colony {} has no units.", colony );

  // 5.1  All colony's units owned by colony.
  for( auto const& p : colony.units_jobs() ) {
    auto unit_id = p.first;
    if( state_for_unit( unit_id ) != e_unit_state::colony )
      return GENERIC_ERROR(
          "{} in Colony {} is not owned by colony.",
          debug_string( unit_id ), colony );
  }

  // 5.2  All units owned by colony are colony's units.
  for( UnitId unit_id : worker_units_from_colony( id ) ) {
    if( !colony.units_jobs().contains( unit_id ) )
      return GENERIC_ERROR(
          "unit {} owned by colony is not in colony {}.",
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
      return GENERIC_ERROR(
          "Colony has nation {} but unit has nation {}.",
          colony.nation(), nation );
  }

  // 8.  Colony's commodity quantites in correct range.
  for( auto comm : enum_traits<e_commodity>::values ) {
    if( colony.commodity_quantity( comm ) < 0 )
      return GENERIC_ERROR(
          "Colony has negative quantity of {}.", comm );
  }

  // 9.  Colony's building set is self-consistent.
  // TODO

  // 10. Unit mfg jobs are self-consistent.
  // TODO

  // 11. Unit land jobs are good.
  // TODO

  // 12. Colony's production status is self-consistent.
  // TODO

  return valid;
}

void check_colony_invariants_die( ColonyId id ) {
  CHECK_HAS_VALUE( check_colony_invariants_safe( id ) );
}

valid_or<e_new_colony_name_err> is_valid_new_colony_name(
    std::string_view name ) {
  if( colony_from_name( name ).has_value() )
    return invalid( e_new_colony_name_err::already_exists );
  return valid;
}

valid_or<e_found_colony_err> unit_can_found_colony(
    UnitId founder ) {
  using Res_t = e_found_colony_err;
  auto& unit  = unit_from_id( founder );

  if( unit.desc().ship )
    return invalid( Res_t::ship_cannot_found_colony );

  if( !unit.is_human() )
    return invalid( Res_t::non_human_cannot_found_colony );

  auto maybe_coord = coord_for_unit_indirect( founder );
  if( !maybe_coord.has_value() )
    return invalid( Res_t::colonist_not_on_map );

  if( colony_from_coord( *maybe_coord ) )
    return invalid( Res_t::colony_exists_here );

  if( !terrain_is_land( *maybe_coord ) )
    return invalid( Res_t::no_water_colony );

  return valid;
}

ColonyId found_colony_unsafe( UnitId           founder,
                              std::string_view name ) {
  if( auto res = is_valid_new_colony_name( name ); !res )
    // FIXME: improve error message generation.
    FATAL( "Cannot found colony, error code: {}.",
           enum_name( res.error() ) );

  if( auto res = unit_can_found_colony( founder ); !res )
    // FIXME: improve error message generation.
    FATAL( "Cannot found colony, error code: {}.",
           enum_name( res.error() ) );

  Unit& unit   = unit_from_id( founder );
  auto  nation = unit.nation();
  UNWRAP_CHECK( where, coord_for_unit_indirect( founder ) );

  // 1. Create colony object.
  UNWRAP_CHECK( col_id,
                cstate_create_colony( nation, where, name ) );
  Colony& col = colony_from_id( col_id );

  // Strip unit of commodities and modifiers and put the commodi-
  // ties into the colony.
  col.strip_unit_commodities( founder );

  // 2. Find initial job for founder. (TODO)
  ColonyJob_t job =
      ColonyJob::mfg{ .mfg_job = e_mfg_job::bells };

  // 3. Move unit into it.
  ustate_change_to_colony( founder, col_id, job );

  // 4. Check colony invariants. Need to abort here if the in-
  // variants are violated because we have already created the
  // colony, and so this should never happen.
  CHECK_HAS_VALUE( check_colony_invariants_safe( col_id ) );

  // 5. Done.
  auto& desc = nation_obj( nation );
  lg.info( "created {} {} colony at {}.", desc.article,
           desc.adjective, where );
  return col_id;
}

waitable<> evolve_colony_one_turn( ColonyId id ) {
  auto& colony = colony_from_id( id );
  lg.debug( "evolving colony: {}.", colony );
  auto& commodities = colony.commodities();
  commodities[e_commodity::food] +=
      rng::between( 3, 7, rng::e_interval::closed );
  if( commodities[e_commodity::food] >= 200 ) {
    commodities[e_commodity::food] -= 200;
    // FIXME: When creating a new unit on the map, decide whether
    // we need to unsentry surrounding units and, if so, find a
    // good place to put that such that it can be reused anytime
    // a unit is created on the map.
    UnitType colonist =
        UnitType::create( e_unit_type::free_colonist );
    auto unit_id = create_unit( colony.nation(), colonist );
    ustate_change_to_map( unit_id, colony.location() );
    co_await landview_ensure_visible( colony.location() );
    ui::e_ok_cancel answer = co_await ui::ok_cancel( fmt::format(
        "The @[H]{}@[] colony has produced a new colonist.  "
        "View colony?",
        colony.name() ) );
    if( answer == ui::e_ok_cancel::ok )
      co_await show_colony_view( id );
  }
  check_colony_invariants_die( id );
}

void change_colony_nation( ColonyId id, e_nation new_nation ) {
  unordered_set<UnitId> units = units_at_or_in_colony( id );
  for( UnitId unit_id : units )
    unit_from_id( unit_id ).change_nation( new_nation );
  auto& colony = colony_from_id( id );
  CHECK( colony.nation() != new_nation );
  colony.set_nation( new_nation );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// FIXME: calling this function on the blinking unit will cause
// errors or check-fails in the game; the proper way to do this
// is to have a mechanism by which we can inject player commands
// as if the player had pressed 'b' so that the game can process
// the fact that the unit in question is now in a colony.
//
// This function is also currently used to setup some colonies
// from Lua at startup, where it works fine. The safer way to do
// that would be to have a single function that both creates a
// unit and the colony together.
LUA_FN( found_colony, ColonyId, UnitId founder,
        std::string const& name ) {
  if( auto res = is_valid_new_colony_name( name ); !res )
    // FIXME: improve error message generation.
    st.error( "cannot found colony here: {}.",
              enum_to_display_name( res.error() ) );
  if( auto res = unit_can_found_colony( founder ); !res )
    st.error( "cannot found colony here." );
  return found_colony_unsafe( founder, name );
}

} // namespace

} // namespace rn
