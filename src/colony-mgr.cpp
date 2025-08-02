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
#include "agents.hpp"
#include "anim-builders.hpp"
#include "co-wait.hpp"
#include "colony-buildings.hpp"
#include "colony-evolve.hpp"
#include "colony-view.hpp"
#include "colony.rds.hpp"
#include "commodity.hpp"
#include "connectivity.hpp"
#include "construction.hpp"
#include "damaged.hpp"
#include "harbor-units.hpp"
#include "iagent.hpp"
#include "immigration.hpp"
#include "land-view.hpp"
#include "map-square.hpp"
#include "native-owned.hpp"
#include "plane-stack.hpp"
#include "revolution-status.hpp"
#include "road.hpp"
#include "roles.hpp"
#include "teaching.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"
#include "unit-transformation.hpp"
#include "visibility.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/nation.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/iter.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/conv.hpp"
#include "base/logger.hpp"
#include "base/scope-exit.hpp"
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <numeric>
#include <ranges>

using namespace std;

namespace rg = std::ranges;

namespace rn {

namespace {

// This returns all units that are either working in the colony
// or who are on the map on the colony square.
unordered_set<UnitId> units_at_or_in_colony(
    Colony const& colony, UnitsState const& units_state ) {
  unordered_set<UnitId> all = units_state.from_colony( colony );
  Coord colony_loc          = colony.location;
  for( GenericUnitId map_id :
       units_state.from_coord( colony_loc ) )
    all.insert( units_state.check_euro_unit( map_id ) );
  return all;
}

ColonyJob find_job_for_initial_colonist( SSConst const& ss,
                                         Player const& player,
                                         Colony const& colony ) {
  refl::enum_map<e_direction, bool> const occupied_squares =
      find_occupied_surrounding_colony_squares( ss, colony );
  // In an unmodded game the colony will not start off with
  // docks, but it could it settings are changed.
  bool has_docks = colony_has_building_level(
      colony, e_colony_building::docks );
  for( e_direction d : refl::enum_values<e_direction> ) {
    if( occupied_squares[d] ) continue;
    Coord const coord = colony.location.moved( d );
    if( !ss.terrain.square_exists( coord ) ) continue;
    MapSquare const& square = ss.terrain.square_at( coord );
    if( is_water( square ) && !has_docks ) continue;
    // Cannot work squares containing LCRs. This is what the
    // original game does, and is probably for two reasons: 1)
    // you cannot see what is under it, and 2) it forces the
    // player to (risk) exploring the LCR if they want to work
    // the square.
    if( square.lost_city_rumor ) continue;
    // Check if there is an indian village on the square.
    if( ss.natives.maybe_dwelling_from_coord( coord )
            .has_value() )
      continue;
    // Check if the land is owned by an indian village.
    if( is_land_native_owned( ss, player, coord ) ) continue;

    // TODO: check if there are any foreign units fortified on
    //       the square.

    // TODO: sanity check that there are no colonies on the
    //       square.

    return ColonyJob::outdoor{ .direction = d,
                               .job = e_outdoor_job::food };
  }

  // Couldn't find a land job, so default to carpenter's shop.
  // This is safe because the carpenter's shop will always be
  // guaranteed to be there, since one can't build any other
  // buildings without it.
  if( !colony.buildings[e_colony_building::carpenters_shop] )
    lg.error( "colony '{}' does not have a carpenter's shop.",
              colony.name );
  return ColonyJob::indoor{ .job = e_indoor_job::hammers };
}

void create_initial_buildings( Colony& colony ) {
  colony.buildings = config_colony.initial_colony_buildings;
}

void clear_abandoned_colony_road( SSConst const& ss,
                                  IMapUpdater& map_updater,
                                  Coord location ) {
  MapSquare const& square = ss.terrain.square_at( location );
  // Depending on how the colony is being destroyed, the road may
  // have already been removed (e.g. for animation purposes), and
  // so if it is already gone then bail early so that we don't
  // have to invoke the map updater to remove the road.
  if( !square.road ) return;

  // As the original game does, we will remove the road that the
  // colony got for free upon its founding. This is for two rea-
  // sons: if we didn't do this then that would enable a "cheat"
  // whereby the player could just keep founding colonies and
  // abandoning them to get free roads (i.e., without expending
  // any tools). Second, it prevents an insightly road island
  // from lingering on the map which the player would otherwise
  // not be able to remove.
  clear_road( map_updater, location );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
ColonyId create_empty_colony( ColoniesState& colonies_state,
                              e_player player, Coord where,
                              string_view name ) {
  return colonies_state.add_colony( Colony{
    .player   = player,
    .name     = string( name ),
    .location = where,
  } );
}

int colony_population( Colony const& colony ) {
  int size = 0;
  for( e_indoor_job job : refl::enum_values<e_indoor_job> )
    size += colony.indoor_jobs[job].size();
  for( e_direction d : refl::enum_values<e_direction> )
    size += colony.outdoor_jobs[d].has_value() ? 1 : 0;
  return size;
}

// Note: this function should produce the workers in a determin-
// istic order, i.e. no relying on hash map iteration.
vector<UnitId> colony_workers( Colony const& colony ) {
  vector<UnitId> res;
  res.reserve( refl::enum_count<e_indoor_job> * 3 +
               refl::enum_count<e_direction> );
  for( e_indoor_job job : refl::enum_values<e_indoor_job> )
    res.insert( res.end(), colony.indoor_jobs[job].begin(),
                colony.indoor_jobs[job].end() );
  for( e_direction d : refl::enum_values<e_direction> )
    if( colony.outdoor_jobs[d].has_value() )
      res.push_back( colony.outdoor_jobs[d]->unit_id );
  rg::sort( res );
  return res;
}

bool colony_has_unit( Colony const& colony, UnitId id ) {
  vector<UnitId> units = colony_units_all( colony );
  return find( units.begin(), units.end(), id ) != units.end();
}

valid_or<e_new_colony_name_err> is_valid_new_colony_name(
    ColoniesState const& colonies_state,
    string_view const name ) {
  if( base::trim( name ) != name )
    return invalid( e_new_colony_name_err::spaces );
  if( colonies_state.maybe_from_name( name ).has_value() )
    return invalid( e_new_colony_name_err::already_exists );
  return valid;
}

valid_or<e_found_colony_err> unit_can_found_colony(
    SSConst const& ss, UnitId founder ) {
  using Res_t      = e_found_colony_err;
  Unit const& unit = ss.units.unit_for( founder );

  UNWRAP_CHECK_T( Player const& player,
                  ss.players.players[unit.player_type()] );
  if( player.revolution.status >=
          e_revolution_status::declared &&
      player.revolution.status < e_revolution_status::won )
    return invalid( Res_t::war_of_independence );

  if( unit.desc().ship )
    return invalid( Res_t::ship_cannot_found_colony );

  if( !unit.is_colonist() )
    return invalid( Res_t::non_colonist_cannot_found_colony );

  if( !can_unit_found( unit.type_obj() ) ) {
    if( unit.type() == e_unit_type::native_convert )
      return invalid( Res_t::native_convert_cannot_found );
    return invalid( Res_t::unit_cannot_found );
  }

  auto maybe_coord =
      coord_for_unit_indirect( ss.units, founder );
  if( !maybe_coord.has_value() )
    return invalid( Res_t::colonist_not_on_map );

  if( ss.colonies.maybe_from_coord( *maybe_coord ) )
    return invalid( Res_t::colony_exists_here );

  // Check if we are too close to another colony.
  for( e_direction d : refl::enum_values<e_direction> ) {
    // Note that at this point we already know that there is no
    // colony on the center square.
    Coord new_coord = maybe_coord->moved( d );
    if( !ss.terrain.square_exists( new_coord ) ) continue;
    if( ss.colonies.maybe_from_coord( maybe_coord->moved( d ) ) )
      return invalid( Res_t::too_close_to_colony );
  }

  if( !ss.terrain.is_land( *maybe_coord ) )
    return invalid( Res_t::no_water_colony );

  if( ss.terrain.square_at( *maybe_coord ).overlay ==
      e_land_overlay::mountains )
    return invalid( Res_t::no_mountain_colony );

  return valid;
}

ColonyId found_colony( SS& ss, TS& ts, Player const& player,
                       UnitId founder, std::string_view name ) {
  if( auto res = is_valid_new_colony_name( ss.colonies, name );
      !res )
    // FIXME: improve error message generation.
    FATAL( "Cannot found colony, error code: {}.",
           refl::enum_value_name( res.error() ) );

  if( auto res = unit_can_found_colony( ss, founder ); !res )
    // FIXME: improve error message generation.
    FATAL( "Cannot found colony, error code: {}.",
           refl::enum_value_name( res.error() ) );

  Unit& unit                 = ss.units.unit_for( founder );
  e_player const player_type = unit.player_type();
  Coord where                = ss.units.coord_for( founder );

  // Create colony object.
  ColonyId col_id = create_empty_colony(
      ss.colonies, player_type, where, name );
  Colony& col = ss.colonies.colony_for( col_id );

  // Populate the colony with the initial set of buildings that
  // are given for free.
  create_initial_buildings( col );

  // Strip unit of commodities and modifiers and put the commodi-
  // ties into the colony.
  strip_unit_to_base_type( ss, ts, unit, col );

  // Find initial job for founder.
  ColonyJob job =
      find_job_for_initial_colonist( ss, player, col );

  // Move unit into it.
  UnitOwnershipChanger const ownership_changer( ss, founder );
  ownership_changer.change_to_colony( ts, col, job );

  // Add road onto colony square.
  set_road( ts.map_updater(), where );

  // The OG does not seem to require the player to pay for this
  // land to found a colony; it just allows the player to found
  // the colony on native-owned land without removing the owned
  // status (and it just ignores the owned status from then on,
  // at least until the colony is abandoned). So land under a
  // colony should always be reported as being not owned by the
  // below function.
  CHECK(
      !is_land_native_owned( ss, player, where ).has_value() );

  // Done.
  auto& desc = player_obj( player_type );
  lg.info( "created {} {} colony at {}.", desc.article,
           player_possessive( player ), where );

  return col_id;
}

void change_colony_player( SS& ss, TS& ts, Colony& colony,
                           e_player new_player ) {
  unordered_set<UnitId> units =
      units_at_or_in_colony( colony, ss.units );
  for( UnitId unit_id : units )
    change_unit_player( ss, ts, ss.units.unit_for( unit_id ),
                        new_player );
  CHECK( colony.player != new_player );
  colony.player = new_player;
}

void strip_unit_to_base_type( SS& ss, TS& ts, Unit& unit,
                              Colony& colony ) {
  UnitTransformation const transform_res =
      strip_to_base_type( unit.composition() );
  change_unit_type( ss, ts, unit, transform_res.new_comp );
  for( auto [type, q] : transform_res.commodity_deltas ) {
    CHECK_GT( q, 0 );
    lg.debug( "adding {} {} to colony {}.", q, type,
              colony.name );
    colony.commodities[type] += q;
  }
}

void add_unit_to_colony_obj_low_level( SS& ss, TS& ts,
                                       Colony& colony,
                                       Unit& unit,
                                       ColonyJob const& job ) {
  CHECK( unit.player_type() == colony.player );
  UnitId const unit_id = unit.id();
  strip_unit_to_base_type( ss, ts, unit, colony );
  // Now add the unit to the colony object.
  SCOPE_EXIT { CHECK( colony.validate() ); };
  CHECK( !colony_has_unit( colony, unit_id ),
         "Unit {} already in colony.", unit_id );
  SWITCH( job ) {
    CASE( indoor ) {
      colony.indoor_jobs[indoor.job].push_back( unit_id );
      sync_colony_teachers( colony );
      break;
    }
    CASE( outdoor ) {
      maybe<OutdoorUnit>& outdoor_unit =
          colony.outdoor_jobs[outdoor.direction];
      CHECK( !outdoor_unit.has_value() );
      outdoor_unit =
          OutdoorUnit{ .unit_id = unit_id, .job = outdoor.job };
      break;
    }
  }
}

void remove_unit_from_colony_obj_low_level( SS& ss,
                                            Colony& colony,
                                            UnitId unit_id ) {
  CHECK( ss.units.unit_for( unit_id ).player_type() ==
         colony.player );
  CHECK( as_const( ss.units )
             .ownership_of( unit_id )
             .holds<UnitOwnership::colony>(),
         "Unit {} is not working in a colony.", unit_id );
  // Now remove the unit from the colony.
  SCOPE_EXIT {
    // If the colony is being abandoned then it will be in an in-
    // valid state transiently.
    if( !colony_workers( colony ).empty() ) {
      CHECK( colony.validate() );
    }
  };

  for( auto& [job, units] : colony.indoor_jobs ) {
    if( find( units.begin(), units.end(), unit_id ) !=
        units.end() ) {
      units.erase( find( units.begin(), units.end(), unit_id ) );
      sync_colony_teachers( colony );
      return;
    }
  }

  for( auto& [direction, outdoor_unit] : colony.outdoor_jobs ) {
    if( outdoor_unit.has_value() &&
        outdoor_unit->unit_id == unit_id ) {
      outdoor_unit = nothing;
      return;
    }
  }

  FATAL( "unit {} not found in colony '{}'.", unit_id,
         colony.name );
}

void change_unit_outdoor_job( Colony& colony, UnitId id,
                              e_outdoor_job new_job ) {
  auto& outdoor_jobs = colony.outdoor_jobs;
  for( e_direction d : refl::enum_values<e_direction> )
    if( outdoor_jobs[d].has_value() )
      if( outdoor_jobs[d]->unit_id == id )
        outdoor_jobs[d]->job = new_job;
}

ColonyDestructionOutcome destroy_colony( SS& ss, TS& ts,
                                         Colony& colony ) {
  ColonyDestructionOutcome outcome;
  // Before the colony is destroyed.
  string const colony_name     = colony.name;
  Coord const colony_location  = colony.location;
  e_player const colony_player = colony.player;
  // These are the units working in the colony, not those at the
  // gate or in cargo.
  vector<UnitId> units = colony_units_all( colony );
  for( UnitId unit_id : units )
    UnitOwnershipChanger( ss, unit_id ).destroy();
  CHECK( colony_population( colony ) == 0 );
  // Remove any units from the cargo of ships, since those ships
  // will be damaged or sunk, and we want to replicate the be-
  // havior of the OG which does not have the concept of units on
  // ships, so those units in the OG would have just been at the
  // gate and sentried. Note that in some cases, such as when a
  // colony is destroyed as a result of an attack, this may have
  // been done in advance, e.g. to offboard any soldiers needed
  // to defend the colony. In that case this just does nothing.
  offboard_units_on_ships( ss, ts, colony_location );
  clear_abandoned_colony_road( ss, ts.map_updater(),
                               colony.location );

  // Send any ships for repair, if a port is available (otherwise
  // the ships will get destroyed). Note that this method handles
  // both the case where a colony is abandoned and the case where
  // it is destroyed in battle. In the former case, the OG actu-
  // ally does not send the ships anywhere; it just leaves them
  // on land. However, when that happens, it then has to deal
  // with the situation where a foreign unit attempts to attack
  // the ship. The OG actually doesn't handle that, it just pan-
  // ics and crashes. In order to avoid the complexity of then
  // dealing with a ship on land, we will just take the position
  // that, when a colony goes away for any reason, any ships left
  // in port will be marked as damaged and returned for repair.
  outcome.port = find_repair_port_for_ship( ss, colony_player,
                                            colony_location );

  // We need to make a copy of this set because we cannot iterate
  // over it while mutating it, which will happen as we move
  // units off of the map and into the harbor.
  unordered_set<GenericUnitId> const at_gate =
      ss.units.from_coord( colony.location );
  for( GenericUnitId generic_id : at_gate ) {
    UnitId const unit_id =
        ss.units.check_euro_unit( generic_id );
    Unit& ship = ss.units.unit_for( unit_id );
    if( !ship.desc().ship ) continue;
    int& count = outcome.ships_that_were_in_port[ship.type()];
    ++count;
    // We should have already removed these above, but just as a
    // sanity check.
    int const num_units_onboard =
        ship.cargo().count_items_of_type<Cargo::unit>();
    CHECK( num_units_onboard == 0,
           "before a colony is destroyed, any units in the "
           "cargo of ships in its port must be removed." );
    if( outcome.port.has_value() )
      move_damaged_ship_for_repair( ss, ts.map_updater(), ship,
                                    *outcome.port );
    else
      UnitOwnershipChanger( ss, ship.id() ).destroy();
  }

  ss.colonies.destroy_colony( colony.id );
  // Note that the player map (fog colony) won't be updated at
  // this point; it will be updated if/when the square that con-
  // tained the colony is transitioned to a fog state.
  return outcome;
}

wait<> run_colony_destruction( SS& ss, TS& ts, Colony& colony,
                               e_ship_damaged_reason reason,
                               maybe<string> msg ) {
  // Must extract this info before destroying the colony.
  string const colony_name     = colony.name;
  e_player const colony_player = colony.player;
  IAgent& agent                = ts.agents()[colony.player];
  // In case it hasn't already been done...
  ColonyDestructionOutcome const outcome =
      destroy_colony( ss, ts, colony );
  if( msg.has_value() ) co_await agent.message_box( *msg );
  // Check if there are any ships in port.
  for( auto [unit_type, count] :
       outcome.ships_that_were_in_port ) {
    if( count == 0 ) continue;
    string const unit_type_name =
        ( count > 1 ) ? unit_attr( unit_type ).name_plural
                      : unit_attr( unit_type ).name;
    string const count_str =
        base::int_to_string_literary( count );
    string const verb = ( count > 1 ) ? "were" : "was";
    if( outcome.port.has_value() ) {
      string const msg = fmt::format(
          "Port in [{}] contained {} [{}] that {} damaged {} "
          "and {} sent to [{}] for repairs.",
          colony_name, count_str, unit_type_name, verb,
          ship_damaged_reason( reason ), verb,
          ship_repair_port_name( ss, colony_player,
                                 *outcome.port ) );
      co_await agent.message_box( msg );
    } else {
      string const msg = fmt::format(
          "Port in [{}] contained {} [{}] that {} damaged {} "
          "and destroyed as there are no available ports for "
          "repair.",
          colony_name, count_str, unit_type_name, verb,
          ship_damaged_reason( reason ) );
      co_await agent.message_box( msg );
    }
  }
}

wait<> run_animated_colony_destruction(
    SS& ss, TS& ts, Colony& colony, e_ship_damaged_reason reason,
    maybe<string> msg ) {
  auto const viz = create_visibility_for(
      ss, player_for_role( ss, e_player_role::viewer ) );
  AnimationSequence const seq =
      anim_seq_for_colony_depixelation( ss, *viz, colony.id );
  co_await ts.planes.get()
      .get_bottom<ILandViewPlane>()
      .animate_if_visible( seq );
  co_await run_colony_destruction( ss, ts, colony, reason, msg );
}

refl::enum_map<e_direction, bool>
find_occupied_surrounding_colony_squares(
    SSConst const& ss, Colony const& colony ) {
  refl::enum_map<e_direction, bool> res;
  Coord const loc = colony.location;
  // We need to search the space of squares just large enough to
  // get any colonies that are two squares away, since those are
  // the only one's whose land squares could overlap with ours.
  Rect const search_space =
      Rect::from( loc - Delta{ .w = 2, .h = 2 },
                  Delta{ .w = 5, .h = 5 } )
          .with_inc_size();
  for( Rect const square : gfx::subrects( search_space ) ) {
    if( !ss.terrain.square_exists( square.upper_left() ) )
      continue;
    if( square.upper_left() == loc ) continue;
    maybe<ColonyId> const colony_id =
        ss.colonies.maybe_from_coord( square.upper_left() );
    if( !colony_id.has_value() ) continue;
    Colony const& other_colony =
        ss.colonies.colony_for( *colony_id );
    for( auto [d, outdoor_unit] : other_colony.outdoor_jobs ) {
      if( outdoor_unit.has_value() ) {
        Coord const occupied = other_colony.location.moved( d );
        maybe<e_direction> const direction_from_our_colony =
            loc.direction_to( occupied );
        if( !direction_from_our_colony.has_value() )
          // The square is not adjacent to our colony.
          continue;
        CHECK( !res[*direction_from_our_colony] );
        res[*direction_from_our_colony] = true;
      }
    }
  }
  return res;
}

void give_stockade_if_needed( Player const& player,
                              Colony& colony ) {
  // Even if the player has a fort (but not a stockade), which
  // could happen via cheat mode or save-file editing, we'll
  // still just test for the stockade anyway.
  if( colony.buildings[e_colony_building::stockade] ) return;
  if( colony_population( colony ) < 3 ) return;
  if( !player.fathers.has[e_founding_father::sieur_de_la_salle] )
    return;
  colony.buildings[e_colony_building::stockade] = true;
}

vector<ColonyId> find_coastal_colonies(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    e_player const player ) {
  vector<ColonyId> colonies = ss.colonies.for_player( player );
  rg::sort( colonies );
  erase_if( colonies, [&]( ColonyId const colony_id ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    bool const coastal   = colony_has_ocean_access(
        ss, connectivity, colony.location );
    return !coastal;
  } );
  return colonies;
}

} // namespace rn
