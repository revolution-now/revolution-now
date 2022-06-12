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
#include "colony-evolve.hpp"
#include "colony-view.hpp"
#include "colony.hpp"
#include "enum.hpp"
#include "game-state.hpp"
#include "gs-colonies.hpp"
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "igui.hpp"
#include "immigration.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "player.hpp"
#include "rand.hpp"
#include "road.hpp"
#include "ustate.hpp"
#include "window.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
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
  unordered_set<UnitId> all =
      units_state.from_colony( colony.id() );
  Coord colony_loc = colony.location();
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
  string msg;

  switch( notification.to_enum() ) {
    case ColonyNotification::e::new_colonist: {
      msg = fmt::format(
          "The @[H]{}@[] colony has produced a new colonist.",
          colony.name() );
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
            spoiled.type, colony.name(), spoiled.quantity );
      } else { // multiple
        msg = fmt::format(
            "Some goods in @[H]{}@[] have exceeded their "
            "warehouse capacities and have been thrown out.",
            colony.name() );
      }
      break;
    }
  }

  if( ask_to_zoom ) {
    vector<ChoiceConfigOption> choices{
        { .key = "no_zoom", .display_name = "Continue turn" },
        { .key = "zoom", .display_name = "Zoom to colony" } };
    string res =
        co_await gui.choice( { .msg     = msg,
                               .options = std::move( choices ),
                               .key_on_escape = "no_zoom" } );
    co_return( res == "zoom" );
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

} // namespace

/****************************************************************
** Public API
*****************************************************************/
ColonyId create_empty_colony( ColoniesState& colonies_state,
                              e_nation nation, Coord where,
                              string_view name ) {
  return colonies_state.add_colony( Colony( wrapped::Colony{
      .nation   = nation,
      .name     = string( name ),
      .location = where,
  } ) );
}

valid_or<e_new_colony_name_err> is_valid_new_colony_name(
    ColoniesState const& colonies_state, string_view name ) {
  if( colonies_state.maybe_from_name( name ).has_value() )
    return invalid( e_new_colony_name_err::already_exists );
  return valid;
}

valid_or<e_found_colony_err> unit_can_found_colony(
    ColoniesState const& colonies_state,
    UnitsState const&    units_state,
    TerrainState const& terrain_state, UnitId founder ) {
  using Res_t      = e_found_colony_err;
  Unit const& unit = units_state.unit_for( founder );

  if( unit.desc().ship )
    return invalid( Res_t::ship_cannot_found_colony );

  if( !unit.is_human() )
    return invalid( Res_t::non_human_cannot_found_colony );

  auto maybe_coord =
      coord_for_unit_indirect( units_state, founder );
  if( !maybe_coord.has_value() )
    return invalid( Res_t::colonist_not_on_map );

  if( colonies_state.maybe_from_coord( *maybe_coord ) )
    return invalid( Res_t::colony_exists_here );

  // Check if we are too close to another colony.
  for( e_direction d : refl::enum_values<e_direction> ) {
    // Note that at this point we already know that there is no
    // colony on the center square.
    Coord new_coord = maybe_coord->moved( d );
    if( !terrain_state.square_exists( new_coord ) ) continue;
    if( colonies_state.maybe_from_coord(
            maybe_coord->moved( d ) ) )
      return invalid( Res_t::too_close_to_colony );
  }

  if( !terrain_state.is_land( *maybe_coord ) )
    return invalid( Res_t::no_water_colony );

  return valid;
}

ColonyId found_colony( ColoniesState&      colonies_state,
                       TerrainState const& terrain_state,
                       UnitsState& units_state, UnitId founder,
                       IMapUpdater& map_updater,
                       string_view  name ) {
  if( auto res =
          is_valid_new_colony_name( colonies_state, name );
      !res )
    // FIXME: improve error message generation.
    FATAL( "Cannot found colony, error code: {}.",
           refl::enum_value_name( res.error() ) );

  if( auto res = unit_can_found_colony(
          colonies_state, units_state, terrain_state, founder );
      !res )
    // FIXME: improve error message generation.
    FATAL( "Cannot found colony, error code: {}.",
           refl::enum_value_name( res.error() ) );

  Unit&    unit   = units_state.unit_for( founder );
  e_nation nation = unit.nation();
  Coord    where  = units_state.coord_for( founder );

  // Create colony object.
  ColonyId col_id =
      create_empty_colony( colonies_state, nation, where, name );
  Colony& col = colonies_state.colony_for( col_id );

  // Strip unit of commodities and modifiers and put the commodi-
  // ties into the colony.
  strip_unit_commodities( unit, col );

  // Find initial job for founder. (TODO)
  ColonyJob_t job =
      ColonyJob::indoor{ .job = e_indoor_job::bells };

  // Move unit into it.
  move_unit_to_colony( units_state, col, founder, job );

  // Add road onto colony square.
  set_road( map_updater, where );

  // Done.
  auto& desc = nation_obj( nation );
  lg.info( "created {} {} colony at {}.", desc.article,
           desc.adjective, where );

  // Let Lua do anything that it needs to the colony.
  CHECK_HAS_VALUE(
      lua_global_state()["colony_mgr"]["on_founded_colony"]
          .pcall( col ) );

  return col_id;
}

void change_colony_nation( Colony&     colony,
                           UnitsState& units_state,
                           e_nation    new_nation ) {
  unordered_set<UnitId> units =
      units_at_or_in_colony( colony, units_state );
  for( UnitId unit_id : units )
    units_state.unit_for( unit_id ).change_nation( new_nation );
  CHECK( colony.nation() != new_nation );
  colony.set_nation( new_nation );
}

void strip_unit_commodities( Unit& unit, Colony& colony ) {
  UnitTransformationResult tranform_res =
      unit.strip_to_base_type();
  for( auto [type, q] : tranform_res.commodity_deltas ) {
    CHECK_GT( q, 0 );
    lg.debug( "adding {} {} to colony {}.", q, type,
              colony.name() );
    colony.commodities()[type] += q;
  }
}

void move_unit_to_colony( UnitsState& units_state,
                          Colony& colony, UnitId unit_id,
                          ColonyJob_t const& job ) {
  Unit& unit = units_state.unit_for( unit_id );
  CHECK( unit.nation() == colony.nation() );
  strip_unit_commodities( unit, colony );
  units_state.change_to_colony( unit_id, colony.id() );
  colony.add_unit( unit_id, job );
}

void remove_unit_from_colony( UnitsState& units_state,
                              Colony& colony, UnitId unit_id ) {
  CHECK( units_state.unit_for( unit_id ).nation() ==
         colony.nation() );
  units_state.disown_unit( unit_id );
  colony.remove_unit( unit_id );
}

wait<> evolve_colonies_for_player(
    LandViewPlane& land_view_plane,
    ColoniesState& colonies_state, SettingsState const& settings,
    UnitsState& units_state, TerrainState const& terrain_state,
    Player& player, IMapUpdater& map_updater, IGui& gui,
    Planes& planes ) {
  e_nation nation = player.nation;
  lg.info( "processing colonies for the {}.", nation );
  queue<ColonyId> colonies;
  for( auto const& [colony_id, colony] : colonies_state.all() )
    if( colony.nation() == nation ) colonies.push( colony_id );
  vector<ColonyEvolution> evolutions;
  while( !colonies.empty() ) {
    ColonyId colony_id = colonies.front();
    colonies.pop();
    Colony& colony = colonies_state.colony_for( colony_id );
    lg.debug( "evolving colony \"{}\".", colony.name() );
    evolutions.push_back( evolve_colony_one_turn(
        colony, settings, units_state, terrain_state, player,
        map_updater ) );
    ColonyEvolution const& ev = evolutions.back();
    if( ev.notifications.empty() ) continue;
    // We have some notifications to present.
    co_await land_view_plane.landview_ensure_visible(
        colony.location() );
    bool zoom_to_colony = co_await present_colony_updates(
        gui, colony, ev.notifications );
    if( zoom_to_colony )
      co_await show_colony_view( planes, colony, terrain_state,
                                 units_state, player );
  }

  // Crosses/immigration.
  CrossesCalculation const crosses_calc =
      compute_crosses( units_state, player.nation );
  give_new_crosses_to_player( player, crosses_calc, evolutions );
  maybe<UnitId> immigrant = co_await check_for_new_immigrant(
      gui, units_state, player, settings,
      crosses_calc.crosses_needed );
  if( immigrant.has_value() )
    lg.info( "a new immigrant ({}) has arrived.",
             units_state.unit_for( *immigrant ).desc().name );
}

} // namespace rn
