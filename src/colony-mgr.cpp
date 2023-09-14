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
#include "anim-builders.hpp"
#include "co-wait.hpp"
#include "colony-buildings.hpp"
#include "colony-evolve.hpp"
#include "colony-view.hpp"
#include "colony.hpp"
#include "commodity.hpp"
#include "construction.hpp"
#include "damaged.hpp"
#include "enum.hpp"
#include "harbor-units.hpp"
#include "igui.hpp"
#include "imap-updater.hpp"
#include "immigration.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "map-square.hpp"
#include "native-owned.hpp"
#include "plane-stack.hpp"
#include "rand.hpp"
#include "road.hpp"
#include "teaching.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/nation.hpp"
#include "config/production.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/iter.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/conv.hpp"
#include "base/keyval.hpp"
#include "base/scope-exit.hpp"
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
  for( GenericUnitId map_id :
       units_state.from_coord( colony_loc ) )
    all.insert( units_state.check_euro_unit( map_id ) );
  return all;
}

struct NotificationMessage {
  string msg;
  bool   transient = false;
};

NotificationMessage generate_colony_notification_message(
    Colony const&             colony,
    ColonyNotification const& notification ) {
  NotificationMessage res{
      // We shouldn't ever use this, but give a fallback to help
      // debugging if we miss something.
      .msg       = base::to_str( notification ),
      .transient = false };

  switch( notification.to_enum() ) {
    case ColonyNotification::e::new_colonist: {
      res.msg = fmt::format(
          "The [{}] colony has produced a new colonist.",
          colony.name );
      break;
    }
    case ColonyNotification::e::colony_starving: {
      res.msg = fmt::format(
          "The [{}] colony is rapidly running out of food.  If "
          "we don't address this soon, some colonists will "
          "starve.",
          colony.name );
      break;
    }
    case ColonyNotification::e::colonist_starved: {
      auto& o = notification
                    .get<ColonyNotification::colonist_starved>();
      res.msg = fmt::format(
          "The [{}] colony has run out of food.  As a result, a "
          "colonist ([{}]) has starved.",
          colony.name, unit_attr( o.type ).name );
      break;
    }
    case ColonyNotification::e::spoilage: {
      auto& o = notification.get<ColonyNotification::spoilage>();
      CHECK( !o.spoiled.empty() );
      if( o.spoiled.size() == 1 ) {
        Commodity const& spoiled = o.spoiled[0];

        res.msg = fmt::format(
            "The store of [{}] in [{}] has exceeded its "
            "warehouse capacity.  [{}] tons have been thrown "
            "out.",
            spoiled.type, colony.name, spoiled.quantity );
      } else { // multiple
        res.msg = fmt::format(
            "Some goods in [{}] have exceeded their warehouse "
            "capacities and have been thrown out.",
            colony.name );
      }
      break;
    }
    case ColonyNotification::e::full_cargo: {
      auto& o =
          notification.get<ColonyNotification::full_cargo>();
      res.msg = fmt::format(
          "A new cargo of [{}] is available in [{}]!",
          lowercase_commodity_display_name( o.what ),
          colony.name );
      break;
    }
    case ColonyNotification::e::construction_missing_tools: {
      auto& o = notification.get<
          ColonyNotification::construction_missing_tools>();
      res.msg = fmt::format(
          "[{}] is in need of [{}] more [tools] to complete its "
          "construction work on the [{}].",
          colony.name, o.need_tools - o.have_tools,
          construction_name( o.what ) );
      break;
    }
    case ColonyNotification::e::construction_complete: {
      auto& o =
          notification
              .get<ColonyNotification::construction_complete>();
      res.msg = fmt::format(
          "[{}] has completed its construction of the [{}]!",
          colony.name, construction_name( o.what ) );
      break;
    }
    case ColonyNotification::e::construction_already_finished: {
      auto& o = notification.get<
          ColonyNotification::construction_already_finished>();
      res.msg = fmt::format(
          "[{}]'s construction of the [{}] has already "
          "completed, we should change its production to "
          "something else.",
          colony.name, construction_name( o.what ) );
      break;
    }
    case ColonyNotification::e::construction_lacking_building: {
      auto& o = notification.get<
          ColonyNotification::construction_lacking_building>();
      res.msg = fmt::format(
          "[{}]'s construction of the [{}] requires the "
          "presence of the [{}] as a prerequisite, and thus "
          "cannot be completed.",
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
      res.msg = fmt::format(
          "[{}]'s construction of the [{}] requires a minimum "
          "population of {}, but only has a population of {}.",
          colony.name, construction_name( o.what ),
          o.required_population, colony_population( colony ) );
      break;
    }
    case ColonyNotification::e::run_out_of_raw_material: {
      auto& o = notification.get<
          ColonyNotification::run_out_of_raw_material>();
      res.msg = fmt::format(
          "[{}] has run out of [{}], Your Excellency.  Our {} "
          "cannot continue production until the supply is "
          "increased.",
          colony.name, o.what,
          config_colony.worker_names_plural[o.job] );
      break;
    }
    case ColonyNotification::e::sons_of_liberty_increased: {
      auto& o = notification.get<
          ColonyNotification::sons_of_liberty_increased>();
      res.msg = fmt::format(
          "Sons of Liberty membership has increased to [{}%] in "
          "[{}]!",
          o.to, colony.name );
      if( o.from < 50 && o.to >= 50 )
        res.msg += fmt::format(
            "  All colonists will now receive a production "
            "bonus: [+{}] for non-expert workers and [+{}] for "
            "expert workers.",
            config_colony.sons_of_liberty_50_bonus_non_expert,
            config_colony.sons_of_liberty_50_bonus_expert );
      if( o.from < 100 && o.to == 100 )
        res.msg += fmt::format(
            "  All colonists will now receive a production "
            "bonus: [+{}] for non-expert workers and [+{}] for "
            "expert workers.",
            config_colony.sons_of_liberty_100_bonus_non_expert,
            config_colony.sons_of_liberty_100_bonus_expert );
      break;
    }
    case ColonyNotification::e::sons_of_liberty_decreased: {
      auto& o = notification.get<
          ColonyNotification::sons_of_liberty_decreased>();
      res.msg = fmt::format(
          "Sons of Liberty membership has decreased to [{}%] in "
          "[{}]!",
          o.to, colony.name );
      if( o.from == 100 && o.to < 100 )
        res.msg += fmt::format(
            "  The production bonus afforded to each colonist "
            "is now reduced." );
      if( o.from >= 50 && o.to < 50 )
        res.msg +=
            "  Colonists will no longer receive any production "
            "bonuses.";
      break;
    }
    case ColonyNotification::e::unit_promoted: {
      auto& o =
          notification.get<ColonyNotification::unit_promoted>();
      res.msg = fmt::format(
          "A colonist in [{}] has learned the specialty "
          "profession [{}]!",
          colony.name, unit_attr( o.promoted_to ).name );
      break;
    }
    case ColonyNotification::e::unit_taught: {
      auto& o =
          notification.get<ColonyNotification::unit_taught>();
      switch( o.from ) {
        case e_unit_type::petty_criminal:
          CHECK( o.to == e_unit_type::indentured_servant );
          res.msg = fmt::format(
              "A [Petty Criminal] in [{}] has been promoted to "
              "[Indentured Servant] through education.",
              colony.name );
          break;
        case e_unit_type::indentured_servant:
          CHECK( o.to == e_unit_type::free_colonist );
          res.msg = fmt::format(
              "An [Indentured Servant] in [{}] has been "
              "promoted to [Free Colonist] through education.",
              colony.name );
          break;
        default:
          res.msg = fmt::format(
              "A [Free Colonist] in [{}] has learned the "
              "specialty profession [{}] through education.",
              colony.name, unit_attr( o.to ).name );
          break;
      }
      break;
    }
    case ColonyNotification::e::teacher_but_no_students: {
      auto& o = notification.get<
          ColonyNotification::teacher_but_no_students>();
      res.msg = fmt::format(
          "We have a teacher in [{}] that is teaching the "
          "specialty profession [{}], but there are no "
          "colonists available to teach.",
          colony.name, unit_attr( o.teacher_type ).name );
      break;
    }
    case ColonyNotification::e::custom_house_sales: {
      auto& o =
          notification
              .get<ColonyNotification::custom_house_sales>();
      string goods;
      for( Invoice const& invoice : o.what )
        goods += fmt::format(
            "{} {} for {} at a {}% charge yielding [{}], ",
            invoice.what.quantity, invoice.what.type,
            invoice.money_delta_before_taxes, invoice.tax_rate,
            invoice.money_delta_final );
      // Remove trailing comma.
      goods.resize( goods.size() - 2 );
      goods += '.';
      res.msg = fmt::format(
          "The [Custom House] in [{}] has sold the following "
          "goods: {}",
          colony.name, goods );
      res.transient = true;
      break;
    }
    case ColonyNotification::e::
        custom_house_selling_boycotted_good: {
      auto& o =
          notification
              .get<ColonyNotification::
                       custom_house_selling_boycotted_good>();
      string goods;
      for( e_commodity const comm : o.what )
        goods += fmt::format(
            "[{}], ", lowercase_commodity_display_name( comm ) );
      // Remove trailing comma.
      goods.resize( goods.size() - 2 );
      res.msg = fmt::format(
          "The [Custom House] in [{}] is unable to sell the "
          "following boycotted goods: {}.  We should stop the "
          "Custom House from attempting to sell them until the "
          "boycott is lifted.",
          colony.name, goods );
      break;
    }
  }
  return res;
}

// Returns true if the user wants to open the colony view.
wait<bool> present_blocking_colony_update(
    IGui& gui, NotificationMessage const& msg,
    bool ask_to_zoom ) {
  CHECK( !msg.transient );
  if( ask_to_zoom ) {
    vector<ChoiceConfigOption> choices{
        { .key = "no_zoom", .display_name = "Continue turn" },
        { .key = "zoom", .display_name = "Zoom to colony" } };
    maybe<string> res = co_await gui.optional_choice(
        { .msg = msg.msg, .options = std::move( choices ) } );
    // If the user hits escape then we don't zoom.
    co_return ( res == "zoom" );
  }
  co_await gui.message_box( msg.msg );
  co_return false;
}

// The idea here (taken from the original game) is that the first
// message will always ask the player if they want to open the
// colony view. If they select no, then any subsequent messages
// will continue to ask them until they select yes. Once they se-
// lect yes (if they ever do) then subsequent messages will still
// be displayed but will not ask them.
wait<bool> present_blocking_colony_updates(
    IGui& gui, vector<NotificationMessage> const& messages ) {
  bool should_zoom = false;
  for( NotificationMessage const& message : messages )
    should_zoom |= co_await present_blocking_colony_update(
        gui, message, !should_zoom );
  co_return should_zoom;
}

// These are messages from all colonies that are to appear in the
// transient pop-up (i.e., the window that is non-blocking, takes
// no input, and fades away on its own).
void present_transient_updates(
    TS& ts, vector<NotificationMessage> const& messages ) {
  for( NotificationMessage const& msg : messages ) {
    CHECK( msg.transient );
    ts.gui.transient_message_box( msg.msg );
  }
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

ColonyJob find_job_for_initial_colonist( SSConst const& ss,
                                         Player const&  player,
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

wait<> run_colony_starvation( SS& ss, TS& ts, Colony& colony ) {
  // Must extract this info before destroying the colony.
  string const msg = fmt::format(
      "[{}] ran out of food and was not able to support "
      "its last remaining colonists.  As a result, the colony "
      "has disappeared.",
      colony.name );
  co_await run_colony_destruction(
      ss, ts, colony, e_ship_damaged_reason::colony_starved,
      msg );
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
  return res;
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
  strip_unit_to_base_type( ss, ts, unit, col );

  // Find initial job for founder.
  ColonyJob job =
      find_job_for_initial_colonist( ss, player, col );

  // Move unit into it.
  move_unit_to_colony( ss, ts, col, founder, job );

  // Add road onto colony square.
  set_road( ts.map_updater, where );

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
  auto& desc = nation_obj( nation );
  lg.info( "created {} {} colony at {}.", desc.article,
           desc.adjective, where );

  return col_id;
}

void change_colony_nation( SS& ss, TS& ts, Colony& colony,
                           e_nation new_nation ) {
  unordered_set<UnitId> units =
      units_at_or_in_colony( colony, ss.units );
  for( UnitId unit_id : units )
    change_unit_nation( ss, ts, ss.units.unit_for( unit_id ),
                        new_nation );
  CHECK( colony.nation != new_nation );
  colony.nation = new_nation;
}

void strip_unit_to_base_type( SS& ss, TS& ts, Unit& unit,
                              Colony& colony ) {
  UnitTransformationResult const transform_res =
      strip_to_base_type( unit.composition() );
  change_unit_type( ss, ts, unit, transform_res.new_comp );
  for( auto [type, q] : transform_res.commodity_deltas ) {
    CHECK_GT( q, 0 );
    lg.debug( "adding {} {} to colony {}.", q, type,
              colony.name );
    colony.commodities[type] += q;
  }
}

void move_unit_to_colony( SS& ss, TS& ts, Colony& colony,
                          UnitId           unit_id,
                          ColonyJob const& job ) {
  Unit& unit = ss.units.unit_for( unit_id );
  CHECK( unit.nation() == colony.nation );
  strip_unit_to_base_type( ss, ts, unit, colony );
  unit_ownership_change_non_interactive(
      ss, unit_id,
      EuroUnitOwnershipChangeTo::colony_low_level{
          .colony_id = colony.id } );
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

void remove_unit_from_colony( SS& ss, Colony& colony,
                              UnitId unit_id ) {
  CHECK( ss.units.unit_for( unit_id ).nation() ==
         colony.nation );
  CHECK( as_const( ss.units )
             .ownership_of( unit_id )
             .holds<UnitOwnership::colony>(),
         "Unit {} is not working in a colony.", unit_id );
  unit_ownership_change_non_interactive(
      ss, unit_id, EuroUnitOwnershipChangeTo::free{} );
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

ColonyDestructionOutcome destroy_colony( SS& ss, TS& ts,
                                         Colony& colony ) {
  ColonyDestructionOutcome outcome;
  // Before the colony is destroyed.
  string const   colony_name     = colony.name;
  Coord const    colony_location = colony.location;
  e_nation const colony_nation   = colony.nation;
  // These are the units working in the colony, not those at the
  // gate or in cargo.
  vector<UnitId> units = colony_units_all( colony );
  for( UnitId unit_id : units ) {
    remove_unit_from_colony( ss, colony, unit_id );
    destroy_unit( ss, unit_id );
  }
  CHECK( colony_population( colony ) == 0 );
  clear_abandoned_colony_road( ss, ts.map_updater,
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
  outcome.port = find_repair_port_for_ship( ss, colony_nation,
                                            colony_location );

  // We need to make s copy of this set because we cannot iterate
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
    int const num_units_onboard =
        ship.cargo().count_items_of_type<Cargo::unit>();
    // This is to ensure that we replicate the behavior of the OG
    // which does not have a concept of units on ships; it just
    // has sentried units whose square conincides with a ship.
    CHECK( num_units_onboard == 0,
           "before a colony is destroyed, any units in the "
           "cargo of ships in its port must be removed." );
    if( outcome.port.has_value() )
      move_damaged_ship_for_repair( ss, ts, ship,
                                    *outcome.port );
    else
      destroy_unit( ss, ship.id() );
  }

  ss.colonies.destroy_colony( colony.id );
  // Now that the colony is gone, update the player's map square
  // so that it no longer has a FogColony on the square.
  Player const& player =
      player_for_nation_or_die( ss.players, colony_nation );
  ts.map_updater.make_squares_visible( player.nation,
                                       { colony_location } );

  UNWRAP_CHECK( player_terrain,
                ss.terrain.player_terrain( player.nation ) );
  // Sanity check. This shouldn't fire given the call above, but
  // you never know.
  CHECK(
      !player_terrain.map[colony_location]->colony.has_value(),
      "the colony {} was destroyed but the player map was not "
      "updated to reflect this.",
      colony_name );
  return outcome;
}

wait<> run_colony_destruction_no_anim(
    SS& ss, TS& ts, Colony& colony, e_ship_damaged_reason reason,
    maybe<string> msg ) {
  // Must extract this info before destroying the colony.
  string const   colony_name   = colony.name;
  e_nation const colony_nation = colony.nation;
  // In case it hasn't already been done...
  clear_abandoned_colony_road( ss, ts.map_updater,
                               colony.location );
  ColonyDestructionOutcome const outcome =
      destroy_colony( ss, ts, colony );
  if( msg.has_value() ) co_await ts.gui.message_box( *msg );
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
          ship_repair_port_name( ss, colony_nation,
                                 *outcome.port ) );
      co_await ts.gui.message_box( msg );
    } else {
      string const msg = fmt::format(
          "Port in [{}] contained {} [{}] that {} damaged {} "
          "and destroyed as there are no available ports for "
          "repair.",
          colony_name, count_str, unit_type_name, verb,
          ship_damaged_reason( reason ) );
      co_await ts.gui.message_box( msg );
    }
  }
}

wait<> run_colony_destruction( SS& ss, TS& ts, Colony& colony,
                               e_ship_damaged_reason reason,
                               maybe<string>         msg ) {
  // The road needs to be cleared before the animation so that
  // the depixelating colony won't reveal a road behind it.
  clear_abandoned_colony_road( ss, ts.map_updater,
                               colony.location );
  AnimationSequence const seq =
      anim_seq_for_colony_depixelation( colony.id );
  co_await ts.planes.land_view().animate( seq );
  co_await run_colony_destruction_no_anim( ss, ts, colony,
                                           reason, msg );
}

wait<> evolve_colonies_for_player( SS& ss, TS& ts,
                                   Player& player ) {
  e_nation nation = player.nation;
  lg.info( "processing colonies for the {}.", nation );
  unordered_map<ColonyId, Colony> const& colonies_all =
      ss.colonies.all();
  vector<ColonyId> colonies;
  colonies.reserve( colonies_all.size() );
  for( auto const& [colony_id, colony] : colonies_all )
    if( colony.nation == nation )
      colonies.push_back( colony_id );
  // This is so that we process them in a deterministic order
  // that doesn't depend on hash map iteration order.
  sort( colonies.begin(), colonies.end() );
  vector<ColonyEvolution> evolutions;
  // These will be accumulated for all colonies and then dis-
  // played at the end.
  vector<NotificationMessage> transient_messages;
  for( ColonyId const colony_id : colonies ) {
    Colony& colony = ss.colonies.colony_for( colony_id );
    lg.debug( "evolving colony \"{}\".", colony.name );
    evolutions.push_back(
        evolve_colony_one_turn( ss, ts, player, colony ) );
    ColonyEvolution const& ev = evolutions.back();
    if( ev.colony_disappeared ) {
      co_await run_colony_starvation( ss, ts, colony );
      // !! at this point the colony will have been deleted, so
      // we should not access it anymore.
      continue;
    }
    if( ev.notifications.empty() ) continue;
    // Separate the transient messages from the blocking mes-
    // sages.
    vector<NotificationMessage> blocking_messages;
    blocking_messages.reserve( ev.notifications.size() );
    for( ColonyNotification const& notification :
         ev.notifications ) {
      NotificationMessage msg =
          generate_colony_notification_message( colony,
                                                notification );
      if( msg.transient )
        transient_messages.push_back( std::move( msg ) );
      else
        blocking_messages.push_back( std::move( msg ) );
    }
    if( !blocking_messages.empty() )
      // We have some blocking notifications to present.
      co_await ts.planes.land_view().ensure_visible(
          colony.location );
    bool const zoom_to_colony =
        co_await present_blocking_colony_updates(
            ts.gui, blocking_messages );
    if( zoom_to_colony ) {
      e_colony_abandoned abandoned =
          co_await ts.colony_viewer.show( ts, colony.id );
      if( abandoned == e_colony_abandoned::yes ) continue;
    }
  }

  // Now that all colonies are done, present all of the transient
  // messages.
  present_transient_updates( ts, transient_messages );

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
                              Colony&       colony ) {
  // Even if the player has a fort (but not a stockade), which
  // could happen via cheat mode or save-file editing, we'll
  // still just test for the stockade anyway.
  if( colony.buildings[e_colony_building::stockade] ) return;
  if( colony_population( colony ) < 3 ) return;
  if( !player.fathers.has[e_founding_father::sieur_de_la_salle] )
    return;
  colony.buildings[e_colony_building::stockade] = true;
}

} // namespace rn
