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
#include "co-wait.hpp"
#include "colony-buildings.hpp"
#include "colony-evolve.hpp"
#include "colony-view.hpp"
#include "colony.hpp"
#include "commodity.hpp"
#include "construction.hpp"
#include "enum.hpp"
#include "igui.hpp"
#include "imap-updater.hpp"
#include "immigration.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "map-square.hpp"
#include "plane-stack.hpp"
#include "rand.hpp"
#include "road.hpp"
#include "teaching.hpp"
#include "ts.hpp"
#include "ustate.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/nation.hpp"
#include "config/production.rds.hpp"
#include "config/unit-type.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/scope-exit.hpp"
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

// base-util
#include "base-util/string.hpp"

using namespace std;

namespace rn {

namespace {

// This returns all units that are either working in the colony
// or who are on the map on the colony square.
unordered_set<UnitId> units_at_or_in_colony(
    Colony const& colony, UnitsState const& units_state ) {
  unordered_set<UnitId> all = units_state.from_colony( colony );
  Coord                 colony_loc = colony.location;
  for( UnitId map_id : units_state.from_coord( colony_loc ) )
    all.insert( map_id );
  return all;
}

// Returns true if the user wants to open the colony view.
//
// TODO: this should probably be moved into a separate module
// when it starts to get large. Also, it will likely need access
// to config files to store the messages.
wait<bool> present_colony_update(
    IGui& gui, Colony const& colony,
    ColonyNotification_t const& notification,
    bool                        ask_to_zoom ) {
  // We shouldn't ever use this, but give a fallback to help de-
  // bugging if we miss something.
  string msg = base::to_str( notification );

  switch( notification.to_enum() ) {
    case ColonyNotification::e::new_colonist: {
      msg = fmt::format(
          "The @[H]{}@[] colony has produced a new colonist.",
          colony.name );
      break;
    }
    case ColonyNotification::e::colonist_starved: {
      auto& o = notification
                    .get<ColonyNotification::colonist_starved>();
      msg = fmt::format(
          "The @[H]{}@[] colony has run out of food.  As a "
          "result, a colonist (@[H]{}@[]) has starved.",
          colony.name, unit_attr( o.type ).name );
      break;
    }
    case ColonyNotification::e::spoilage: {
      auto& o = notification.get<ColonyNotification::spoilage>();
      CHECK( !o.spoiled.empty() );
      if( o.spoiled.size() == 1 ) {
        Commodity const& spoiled = o.spoiled[0];

        msg = fmt::format(
            "The store of @[H]{}@[] in @[H]{}@[] has exceeded "
            "its warehouse capacity.  @[H]{}@[] tons have been "
            "thrown out.",
            spoiled.type, colony.name, spoiled.quantity );
      } else { // multiple
        msg = fmt::format(
            "Some goods in @[H]{}@[] have exceeded their "
            "warehouse capacities and have been thrown out.",
            colony.name );
      }
      break;
    }
    case ColonyNotification::e::full_cargo: {
      auto& o =
          notification.get<ColonyNotification::full_cargo>();
      msg = fmt::format(
          "A new cargo of @[H]{}@[] is available in @[H]{}@[]!",
          lowercase_commodity_display_name( o.what ), colony.name );
      break;
    }
    case ColonyNotification::e::construction_missing_tools: {
      auto& o = notification.get<
          ColonyNotification::construction_missing_tools>();
      msg = fmt::format(
          "@[H]{}@[] is in need of @[H]{}@[] more @[H]tools@[] "
          "to complete its construction work on the @[H]{}@[].",
          colony.name, o.need_tools - o.have_tools,
          construction_name( o.what ) );
      break;
    }
    case ColonyNotification::e::construction_complete: {
      auto& o =
          notification
              .get<ColonyNotification::construction_complete>();
      msg = fmt::format(
          "@[H]{}@[] has completed its construction of the "
          "@[H]{}@[]!",
          colony.name, construction_name( o.what ) );
      break;
    }
    case ColonyNotification::e::construction_already_finished: {
      auto& o = notification.get<
          ColonyNotification::construction_already_finished>();
      msg = fmt::format(
          "@[H]{}@[]'s construction of the @[H]{}@[] has "
          "already completed, we should change its production "
          "to something else.",
          colony.name, construction_name( o.what ) );
      break;
    }
    case ColonyNotification::e::construction_lacking_building: {
      auto& o = notification.get<
          ColonyNotification::construction_lacking_building>();
      msg = fmt::format(
          "@[H]{}@[]'s construction of the @[H]{}@[] requires "
          "the presence of the @[H]{}@[] as a prerequisite, and "
          "thus cannot be completed.",
          colony.name, construction_name( o.what ),
          construction_name( Construction::building{
              .what = o.required_building } ) );
      break;
    }
    // clang-format off
    case ColonyNotification::e::construction_lacking_population: {
      // clang-format on
      auto& o = notification.get<
          ColonyNotification::construction_lacking_population>();
      msg = fmt::format(
          "@[H]{}@[]'s construction of the @[H]{}@[] requires a "
          "minimum population of {}, but only has a population "
          "of {}.",
          colony.name, construction_name( o.what ),
          o.required_population, colony_population( colony ) );
      break;
    }
    case ColonyNotification::e::run_out_of_raw_material: {
      auto& o = notification.get<
          ColonyNotification::run_out_of_raw_material>();
      msg = fmt::format(
          "@[H]{}@[] has run out of @[H]{}@[], Your Excellency. "
          " Our {} cannot continue production until the supply "
          "is increased.",
          colony.name, o.what,
          config_colony.worker_names_plural[o.job] );
      break;
    }
    case ColonyNotification::e::sons_of_liberty_increased: {
      auto& o = notification.get<
          ColonyNotification::sons_of_liberty_increased>();
      msg = fmt::format(
          "Sons of Liberty membership has increased to "
          "@[H]{}%@[] in @[H]{}@[]!",
          o.to, colony.name );
      if( o.from < 50 && o.to >= 50 )
        msg += fmt::format(
            "  All colonists will now receive a production "
            "bonus: @[H]+{}@[] for non-expert workers and "
            "@[H]+{}@[] for expert workers.",
            config_colony.sons_of_liberty_50_bonus_non_expert,
            config_colony.sons_of_liberty_50_bonus_expert );
      if( o.from < 100 && o.to == 100 )
        msg += fmt::format(
            "  All colonists will now receive a production "
            "bonus: @[H]+{}@[] for non-expert workers and "
            "@[H]+{}@[] for expert workers.",
            config_colony.sons_of_liberty_100_bonus_non_expert,
            config_colony.sons_of_liberty_100_bonus_expert );
      break;
    }
    case ColonyNotification::e::sons_of_liberty_decreased: {
      auto& o = notification.get<
          ColonyNotification::sons_of_liberty_decreased>();
      msg = fmt::format(
          "Sons of Liberty membership has decreased to "
          "@[H]{}%@[] in @[H]{}@[]!",
          o.to, colony.name );
      if( o.from == 100 && o.to < 100 )
        msg += fmt::format(
            "  The production bonus afforded to each colonist "
            "is now reduced." );
      if( o.from >= 50 && o.to < 50 )
        msg +=
            "  Colonists will no longer receive any production "
            "bonuses.";
      break;
    }
    case ColonyNotification::e::unit_promoted: {
      auto& o =
          notification.get<ColonyNotification::unit_promoted>();
      msg = fmt::format(
          "A colonist in @[H]{}@[] has learned the specialty "
          "profession @[H]{}@[]!",
          colony.name, unit_attr( o.promoted_to ).name );
      break;
    }
    case ColonyNotification::e::unit_taught: {
      auto& o =
          notification.get<ColonyNotification::unit_taught>();
      switch( o.from ) {
        case e_unit_type::petty_criminal:
          CHECK( o.to == e_unit_type::indentured_servant );
          msg = fmt::format(
              "A @[H]Petty Criminal@[] in @[H]{}@[] has been "
              "promoted to @[H]Indentured Servant@[] through "
              "education.",
              colony.name );
          break;
        case e_unit_type::indentured_servant:
          CHECK( o.to == e_unit_type::free_colonist );
          msg = fmt::format(
              "An @[H]Indentured Servant@[] in @[H]{}@[] has "
              "been promoted to @[H]Free Colonist@[] through "
              "education.",
              colony.name );
          break;
        default:
          msg = fmt::format(
              "A @[H]Free Colonist@[] in @[H]{}@[] has learned "
              "the specialty profession @[H]{}@[] through "
              "education.",
              colony.name, unit_attr( o.to ).name );
          break;
      }
      break;
    }
    case ColonyNotification::e::teacher_but_no_students: {
      auto& o = notification.get<
          ColonyNotification::teacher_but_no_students>();
      msg = fmt::format(
          "We have a teacher in @[H]{}@[] that is teaching the "
          "specialty "
          "profession @[H]{}@[], but there are no colonists "
          "available to teach.",
          colony.name, unit_attr( o.teacher_type ).name );
      break;
    }
  }

  if( ask_to_zoom ) {
    vector<ChoiceConfigOption> choices{
        { .key = "no_zoom", .display_name = "Continue turn" },
        { .key = "zoom", .display_name = "Zoom to colony" } };
    maybe<string> res = co_await gui.optional_choice(
        { .msg = msg, .options = std::move( choices ) } );
    // If the user hits escape then we don't zoom.
    co_return ( res == "zoom" );
  }
  co_await gui.message_box( msg );
  co_return false;
}

// The idea here (taken from the original game) is that the first
// message will always ask the player if they want to open the
// colony view. If they select no, then any subsequent messages
// will continue to ask them until they select yes. Once they se-
// lect yes (if they ever do) then subsequent messages will still
// be displayed but will not ask them.
wait<bool> present_colony_updates(
    IGui& gui, Colony const& colony,
    vector<ColonyNotification_t> const& notifications ) {
  bool should_zoom = false;
  for( ColonyNotification_t const& notification : notifications )
    should_zoom |= co_await present_colony_update(
        gui, colony, notification, !should_zoom );
  co_return should_zoom;
}

void give_new_crosses_to_player(
    Player& player, CrossesCalculation const& crosses_calc,
    vector<ColonyEvolution> const& evolutions ) {
  int const colonies_crosses =
      accumulate( evolutions.begin(), evolutions.end(), 0,
                  []( int so_far, ColonyEvolution const& ev ) {
                    return so_far + ev.production.crosses;
                  } );
  add_player_crosses( player, colonies_crosses,
                      crosses_calc.dock_crosses_bonus );
}

ColonyJob_t find_job_for_initial_colonist(
    SSConst const& ss, Colony const& colony ) {
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
    // TODO: Check if there is an indian village on the square.

    // TODO: Check if the land is owned by an indian village.

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

wait<> run_colony_starvation( Planes& planes, SS& ss, TS& ts,
                              Colony& colony ) {
  // Must extract this info before destroying the colony.
  string const msg = fmt::format(
      "@[H]{}@[] ran out of food and was not able to support "
      "its last remaining colonists.  As a result, the colony "
      "has disappeared.",
      colony.name );
  co_await run_colony_destruction( planes, ss, ts, colony, msg );
  // !! Do not reference `colony` beyond this point.
}

void clear_abandoned_colony_road( SSConst const& ss,
                                  IMapUpdater&   map_updater,
                                  Coord          location ) {
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
                              e_nation nation, Coord where,
                              string_view name ) {
  return colonies_state.add_colony( Colony{
      .nation   = nation,
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

bool colony_has_unit( Colony const& colony, UnitId id ) {
  vector<UnitId> units = colony_units_all( colony );
  return find( units.begin(), units.end(), id ) != units.end();
}

valid_or<e_new_colony_name_err> is_valid_new_colony_name(
    ColoniesState const& colonies_state, string_view name ) {
  if( colonies_state.maybe_from_name( name ).has_value() )
    return invalid( e_new_colony_name_err::already_exists );
  return valid;
}

valid_or<e_found_colony_err> unit_can_found_colony(
    SSConst const& ss, UnitId founder ) {
  using Res_t      = e_found_colony_err;
  Unit const& unit = ss.units.unit_for( founder );

  if( unit.desc().ship )
    return invalid( Res_t::ship_cannot_found_colony );

  if( !unit.is_human() )
    return invalid( Res_t::non_human_cannot_found_colony );

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

ColonyId found_colony( SS& ss, TS& ts, UnitId founder,
                       std::string_view name ) {
  if( auto res = is_valid_new_colony_name( ss.colonies, name );
      !res )
    // FIXME: improve error message generation.
    FATAL( "Cannot found colony, error code: {}.",
           refl::enum_value_name( res.error() ) );

  if( auto res = unit_can_found_colony( ss, founder ); !res )
    // FIXME: improve error message generation.
    FATAL( "Cannot found colony, error code: {}.",
           refl::enum_value_name( res.error() ) );

  Unit&    unit   = ss.units.unit_for( founder );
  e_nation nation = unit.nation();
  Coord    where  = ss.units.coord_for( founder );

  // Create colony object.
  ColonyId col_id =
      create_empty_colony( ss.colonies, nation, where, name );
  Colony& col = ss.colonies.colony_for( col_id );

  // Populate the colony with the initial set of buildings that
  // are given for free.
  create_initial_buildings( col );

  // Strip unit of commodities and modifiers and put the commodi-
  // ties into the colony.
  strip_unit_to_base_type( unit, col );

  // Find initial job for founder. (TODO)
  ColonyJob_t job = find_job_for_initial_colonist( ss, col );

  // Move unit into it.
  move_unit_to_colony( ss.units, col, founder, job );

  // Add road onto colony square.
  set_road( ts.map_updater, where );

  // Done.
  auto& desc = nation_obj( nation );
  lg.info( "created {} {} colony at {}.", desc.article,
           desc.adjective, where );

  return col_id;
}

void change_colony_nation( Colony&     colony,
                           UnitsState& units_state,
                           e_nation    new_nation ) {
  unordered_set<UnitId> units =
      units_at_or_in_colony( colony, units_state );
  for( UnitId unit_id : units )
    units_state.unit_for( unit_id ).change_nation( units_state,
                                                   new_nation );
  CHECK( colony.nation != new_nation );
  colony.nation = new_nation;
}

void strip_unit_to_base_type( Unit& unit, Colony& colony ) {
  UnitTransformationResult tranform_res =
      unit.strip_to_base_type();
  for( auto [type, q] : tranform_res.commodity_deltas ) {
    CHECK_GT( q, 0 );
    lg.debug( "adding {} {} to colony {}.", q, type,
              colony.name );
    colony.commodities[type] += q;
  }
}

void move_unit_to_colony( UnitsState& units_state,
                          Colony& colony, UnitId unit_id,
                          ColonyJob_t const& job ) {
  Unit& unit = units_state.unit_for( unit_id );
  CHECK( unit.nation() == colony.nation );
  strip_unit_to_base_type( unit, colony );
  units_state.change_to_colony( unit_id, colony.id );
  // Now add the unit to the colony.
  SCOPE_EXIT( CHECK( colony.validate() ) );
  CHECK( !colony_has_unit( colony, unit_id ),
         "Unit {} already in colony.", unit_id );
  switch( job.to_enum() ) {
    case ColonyJob::e::indoor: {
      auto const& o = job.get<ColonyJob::indoor>();
      colony.indoor_jobs[o.job].push_back( unit_id );
      sync_colony_teachers( colony );
      break;
    }
    case ColonyJob::e::outdoor: {
      auto const&         o = job.get<ColonyJob::outdoor>();
      maybe<OutdoorUnit>& outdoor_unit =
          colony.outdoor_jobs[o.direction];
      CHECK( !outdoor_unit.has_value() );
      outdoor_unit =
          OutdoorUnit{ .unit_id = unit_id, .job = o.job };
      break;
    }
  }
}

void remove_unit_from_colony( UnitsState& units_state,
                              Colony& colony, UnitId unit_id ) {
  CHECK( units_state.unit_for( unit_id ).nation() ==
         colony.nation );
  units_state.disown_unit( unit_id );
  // Now remove the unit from the colony.
  SCOPE_EXIT( CHECK( colony.validate() ) );

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

void destroy_colony( SS& ss, IMapUpdater& map_updater,
                     Colony& colony ) {
  // These are the units working in the colony, not those at the
  // gate or in port (which won't be affected).
  vector<UnitId> units = colony_units_all( colony );
  for( UnitId unit_id : units ) {
    remove_unit_from_colony( ss.units, colony, unit_id );
    ss.units.destroy_unit( unit_id );
  }
  CHECK( colony_population( colony ) == 0 );
  clear_abandoned_colony_road( ss, map_updater,
                               colony.location );
  // Should be last.
  ss.colonies.destroy_colony( colony.id );
}

wait<> run_colony_destruction( Planes& planes, SS& ss, TS& ts,
                               Colony&       colony,
                               maybe<string> msg ) {
  // Must extract this info before destroying the colony.
  string const name     = colony.name;
  Coord const  location = colony.location;
  clear_abandoned_colony_road( ss, ts.map_updater,
                               colony.location );
  co_await planes.land_view()
      .landview_animate_colony_depixelation( colony );
  destroy_colony( ss, ts.map_updater, colony );
  if( msg.has_value() ) co_await ts.gui.message_box( *msg );
  // Check if there are any ships in port.
  unordered_set<UnitId> const& at_gate =
      ss.units.from_coord( location );
  for( UnitId unit_id : at_gate ) {
    if( !ss.units.unit_for( unit_id ).desc().ship ) continue;
    string const msg = fmt::format(
        "@[H]{}@[] had ships in its port that are now exposed "
        "on land.  We should move them into the ocean soon "
        "since they are vulnerable and weak while in the "
        "abandoned colony port.",
        name );
    co_await ts.gui.message_box( msg );
    break;
  }
}

wait<> evolve_colonies_for_player( Planes& planes, SS& ss,
                                   TS& ts, Player& player ) {
  e_nation nation = player.nation;
  lg.info( "processing colonies for the {}.", nation );
  queue<ColonyId> colonies;
  for( auto const& [colony_id, colony] : ss.colonies.all() )
    if( colony.nation == nation ) colonies.push( colony_id );
  vector<ColonyEvolution> evolutions;
  while( !colonies.empty() ) {
    ColonyId colony_id = colonies.front();
    colonies.pop();
    Colony& colony = ss.colonies.colony_for( colony_id );
    lg.debug( "evolving colony \"{}\".", colony.name );
    evolutions.push_back(
        evolve_colony_one_turn( ss, ts, colony ) );
    ColonyEvolution const& ev = evolutions.back();
    if( ev.colony_disappeared ) {
      co_await run_colony_starvation( planes, ss, ts, colony );
      // !! at this point the colony will have been deleted, so
      // we should not access it anymore.
      continue;
    }
    if( ev.notifications.empty() ) continue;
    // We have some notifications to present.
    co_await planes.land_view().landview_ensure_visible(
        colony.location );
    bool zoom_to_colony = co_await present_colony_updates(
        ts.gui, colony, ev.notifications );
    if( zoom_to_colony ) {
      e_colony_abandoned abandoned =
          co_await show_colony_view( planes, ss, ts, colony );
      if( abandoned == e_colony_abandoned::yes ) continue;
    }
  }

  // Crosses/immigration.
  CrossesCalculation const crosses_calc =
      compute_crosses( ss.units, player.nation );
  give_new_crosses_to_player( player, crosses_calc, evolutions );
  maybe<UnitId> immigrant = co_await check_for_new_immigrant(
      ss, ts, player, crosses_calc.crosses_needed );
  if( immigrant.has_value() )
    lg.info( "a new immigrant ({}) has arrived.",
             ss.units.unit_for( *immigrant ).desc().name );
}

refl::enum_map<e_direction, bool>
find_occupied_surrounding_colony_squares(
    SSConst const& ss, Colony const& colony ) {
  refl::enum_map<e_direction, bool> res;
  Coord const                       loc = colony.location;
  // We need to search the space of squares just large enough to
  // get any colonies that are two squares away, since those are
  // the only one's whose land squares could overlap with ours.
  Rect const search_space =
      Rect::from( loc - Delta{ .w = 2, .h = 2 },
                  Delta{ .w = 5, .h = 5 } )
          .with_inc_size();
  for( Coord square : search_space ) {
    if( !ss.terrain.square_exists( square ) ) continue;
    if( square == loc ) continue;
    maybe<ColonyId> const colony_id =
        ss.colonies.maybe_from_coord( square );
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

} // namespace rn
