/****************************************************************
**trade-route.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Handles things related to trade routes.
*
*****************************************************************/
#include "trade-route.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "commodity.hpp"
#include "connectivity.hpp"
#include "goto-registry.hpp"
#include "goto.hpp"
#include "harbor-units.hpp"
#include "iagent.hpp"
#include "igui.hpp"
#include "market.hpp"
#include "trade-route-ui.hpp"
#include "unit-mgr.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/trade.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/nation.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/trade-route.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// base
#include "base/logger.hpp"

// C++ standard library
#include <ranges>

using namespace std;

namespace rg = std::ranges;

namespace rn {

namespace {

using ::gfx::point;
using ::refl::enum_count;
using ::refl::enum_map;
using ::refl::enum_values;

[[nodiscard]] bool trade_route_name_exists(
    TradeRouteState const& trade_routes, string const& name ) {
  for( auto const& [route_id, route] : trade_routes.routes )
    if( route.name == name ) //
      return true;
  return false;
}

string trade_route_name_suggestion(
    TradeRouteState const& trade_routes,
    string const& colony_name ) {
  string suggestion;
  auto const& suffixes =
      config_trade.trade_routes.name_suggestion_suffixes;
  int const num_suffixes = suffixes.size();
  if( num_suffixes == 0 ) {
    suggestion = format( "{} Route", colony_name );
    return suggestion;
  }
  auto const get_suffix = [&]( int const idx ) {
    CHECK_GE( idx, 0 );
    CHECK_LT( idx, ssize( suffixes ) );
    return suffixes[idx];
  };
  int const start = trade_routes.last_trade_route_id;
  auto const use  = [&]( string const& suffix ) {
    return format( "{} {}", colony_name, suffix );
  };
  for( int i = 0; i < num_suffixes; ++i ) {
    int const idx = ( start + i ) % num_suffixes;
    suggestion    = use( get_suffix( idx ) );
    if( !trade_route_name_exists( trade_routes, suggestion ) )
      return suggestion;
  }
  // We can't find a straightforward combination that hasn't al-
  // ready been used, so we need to start appending numbers.
  string const suffix = get_suffix( start % num_suffixes );
  for( int iteration = 2;; ++iteration ) {
    suggestion = use( format( "{} {}", suffix, iteration ) );
    if( !trade_route_name_exists( trade_routes, suggestion ) )
      return suggestion;
  }
}

wait<maybe<TradeRouteId>> ask_select_trade_route(
    SSConst const& ss, Player const& player, IGui& gui,
    string const& title ) {
  auto const& routes = ss.trade_routes;
  ChoiceConfig config;
  config.msg = title;
  for( int i = 1;
       auto const& [route_id, route] : routes.routes ) {
    if( route.player != player.type ) continue;
    config.options.push_back( ChoiceConfigOption{
      .key          = to_string( route_id ),
      .display_name = format( "{}. {}", i++, route.name ) } );
  }
  if( config.options.empty() ) {
    co_await gui.message_box(
        "You have not yet defined any trade routes." );
    co_return nothing;
  }
  co_return co_await gui.optional_choice_int_key( config );
}

[[nodiscard]] e_trade_route_type trade_route_type_for_unit(
    Unit const& unit ) {
  using enum e_trade_route_type;
  return unit.desc().ship ? sea : land;
}

} // namespace

/****************************************************************
** Sanitization.
*****************************************************************/
struct TradeRoutesSanitizedToken {
  int authentic = 123456;

 private:
  friend TradeRoutesSanitizedToken const& sanitize_trade_routes(
      SSConst const& ss, Player const& player,
      TradeRouteState& trade_routes,
      vector<TradeRouteSanitizationAction>& actions_taken );

  TradeRoutesSanitizedToken() = default;
};

void validate_token( TradeRoutesSanitizedToken const& token ) {
  CHECK( token.authentic == 123456,
         "invalid trade route sanitization token." );
}

// Returning nothing means the stop is ok.
static maybe<TradeRouteSanitizationAction>
trade_route_stop_inaccessible( SSConst const& ss,
                               Player const& player,
                               TradeRoute const& route,
                               TradeRouteTarget const& target ) {
  using Action = TradeRouteSanitizationAction;
  SWITCH( target ) {
    CASE( colony ) {
      if( !ss.colonies.exists( colony.colony_id ) )
        return Action::colony_no_longer_exists{ .route_name =
                                                    route.name };
      Colony const& colony_o =
          ss.colonies.colony_for( colony.colony_id );
      if( colony_o.player != player.type )
        return Action::colony_changed_player{
          .colony_id  = colony.colony_id,
          .route_name = route.name };
      break;
    }
    CASE( harbor ) {
      if( player.revolution.status >=
          e_revolution_status::declared )
        return Action::no_harbor_post_declaration{
          .route_name = route.name };
      break;
    }
  }
  return nothing;
}

TradeRoutesSanitizedToken const& sanitize_trade_routes(
    SSConst const& ss, Player const& player,
    TradeRouteState& trade_routes,
    vector<TradeRouteSanitizationAction>& actions_taken ) {
  using Action = TradeRouteSanitizationAction;
  static TradeRoutesSanitizedToken const kToken;
  actions_taken.clear();

  // Cap number of stops to four. This isn't really necessary be-
  // cause both the editing UI and the save state validation will
  // ensure that there are never more than four stops, but we'll
  // be defensive.
  for( auto& [route_id, route] : trade_routes.routes ) {
    if( route.player != player.type ) continue;
    if( route.stops.size() <= 4 ) continue;
    route.stops.resize( 4 );
    actions_taken.push_back(
        Action::too_many_stops{ .route_name = route.name } );
  }

  // Erase inaccessible stops.
  for( auto& [route_id, route] : trade_routes.routes ) {
    if( route.player != player.type ) continue;
    bool const needs_removing = [&] {
      for( TradeRouteStop const& stop : route.stops )
        if( trade_route_stop_inaccessible( ss, player, route,
                                           stop.target ) )
          return true;
      return false;
    }();
    if( !needs_removing ) continue;
    vector<TradeRouteStop> new_stops;
    new_stops.reserve( route.stops.size() );
    for( TradeRouteStop const& stop : route.stops ) {
      auto const reason = trade_route_stop_inaccessible(
          ss, player, route, stop.target );
      if( reason.has_value() ) {
        actions_taken.push_back( *reason );
        continue;
      }
      new_stops.push_back( stop );
    }
    route.stops = new_stops;
  }

  // Erase empty routes. NOTE: must do this last in case the
  // above steps made changes that yielded empty routes.
  vector<pair<int, string>> empty;
  for( auto const& [id, route] : trade_routes.routes ) {
    if( route.player != player.type ) continue;
    if( route.stops.empty() )
      empty.push_back( pair{ id, route.name } );
  }
  for( auto const& [id, name] : empty ) {
    actions_taken.push_back(
        TradeRouteSanitizationAction::empty_route{ .route_name =
                                                       name } );
    trade_routes.routes.erase( id );
  }

  return kToken;
}

wait<> show_sanitization_actions(
    SSConst const& ss, IGui& gui, IAgent& agent,
    vector<TradeRouteSanitizationAction> const& actions_taken,
    TradeRoutesSanitizedToken const& token ) {
  validate_token( token );
  if( actions_taken.empty() ) co_return;
  vector<string> msgs;
  // NOTE: we should not assume that any trade route referenced
  // still exists, since the sanitization action could have per-
  // formed an action on a trade route (such as removing
  // colonies) which left it empty in which case it will have
  // been removed.
  for( TradeRouteSanitizationAction const& action :
       actions_taken ) {
    SWITCH( action ) {
      CASE( colony_no_longer_exists ) {
        msgs.push_back(
            format( "A colony listed as a stop on the [{}] "
                    "trade route has been removed from the "
                    "itinerary as it no longer exists.",
                    colony_no_longer_exists.route_name ) );
        break;
      }
      CASE( colony_changed_player ) {
        Colony const& colony = ss.colonies.colony_for(
            colony_changed_player.colony_id );
        msgs.push_back( format(
            "The colony of [{}], listed as a stop on the [{}] "
            "trade route, has been removed from the itinerary "
            "as it is now occupied by the [{}].",
            colony.name, colony_changed_player.route_name,
            config_nation.players[colony.player]
                .display_name_pre_declaration ) );
        break;
      }
      CASE( no_harbor_post_declaration ) {
        msgs.push_back(
            format( "All European Harbor stops on the [{}] "
                    "trade route have been removed as the "
                    "European Harbor is no longer accessible "
                    "after the War of Independence begins.",
                    no_harbor_post_declaration.route_name ) );
        break;
      }
      CASE( empty_route ) {
        msgs.push_back( format(
            "The [{}] trade route has been left with no stops "
            "on its itinerary and has thus been deleted.",
            empty_route.route_name ) );
        break;
      }
      CASE( too_many_stops ) {
        msgs.push_back(
            format( "The [{}] trade route has exceeded the "
                    "limit of four stops. All stops beyond the "
                    "fourth have thus been removed.",
                    too_many_stops.route_name ) );
        break;
      }
    }
  }
  for( bool sleep = false; string const& msg : msgs ) {
    co_await agent.message_box( "Trade Route Alert: {}", msg );
    if( sleep && agent.human() )
      co_await gui.wait_for( chrono::milliseconds{ 200 } );
    sleep = true;
  }
  co_return;
}

wait<std::reference_wrapper<TradeRoutesSanitizedToken const>>
run_trade_route_sanitization( SSConst const& ss,
                              Player const& player, IGui& gui,
                              TradeRouteState& trade_routes,
                              IAgent& agent ) {
  vector<TradeRouteSanitizationAction> actions_taken;
  TradeRoutesSanitizedToken const& token = sanitize_trade_routes(
      ss, player, trade_routes, actions_taken );
  co_await show_sanitization_actions(
      ss, gui, agent, as_const( actions_taken ), token );
  co_return token;
}

maybe<unit_orders::trade_route> sanitize_unit_trade_route_orders(
    SSConst const& ss, Unit const& unit,
    TradeRoutesSanitizedToken const& token ) {
  validate_token( token );
  UNWRAP_RETURN_T(
      unit_orders::trade_route const& orders,
      unit.orders().get_if<unit_orders::trade_route>() );
  auto const route =
      look_up_trade_route( ss.trade_routes, orders.id );
  if( !route.has_value() ) return nothing;
  if( route->type != trade_route_type_for_unit( unit ) )
    return nothing;
  // This can happen if the wagon train is captured in a colony
  // while on a trade route.
  if( route->player != unit.player_type() ) return nothing;
  // This should not happen as it should have been caught in san-
  // itization, but lets be defensive. In particular, this is
  // just in case there is a weird edge case that we're not
  // thinking of where the unit in question changes nation while
  // it is on a trade route with no stops and then only the new
  // nation's trade routes get sanitized, thus leaving it still
  // pointing to a trade route with no stops. There is another
  // safety against this which is that we check the nation of the
  // unit above, so really that should not happen either, but you
  // never know.
  if( route->stops.empty() ) return nothing;
  unit_orders::trade_route new_orders = orders;
  if( new_orders.en_route_to_stop >= ssize( route->stops ) )
    new_orders.en_route_to_stop = 0;
  return new_orders;
}

/****************************************************************
** Querying.
*****************************************************************/
maybe<TradeRoute&> look_up_trade_route(
    TradeRouteState& trade_routes, TradeRouteId const id ) {
  CHECK_GT( id, 0 ); // zero is always an invalid value.
  CHECK_LE( id, trade_routes.last_trade_route_id );
  auto const iter = trade_routes.routes.find( id );
  if( iter == trade_routes.routes.end() ) return nothing;
  return iter->second;
}

maybe<TradeRoute const&> look_up_trade_route(
    TradeRouteState const& trade_routes,
    TradeRouteId const id ) {
  CHECK_GT( id, 0 ); // zero is always an invalid value.
  CHECK_LE( id, trade_routes.last_trade_route_id );
  auto const iter = trade_routes.routes.find( id );
  if( iter == trade_routes.routes.end() ) return nothing;
  return iter->second;
}

maybe<TradeRouteStop const&> look_up_trade_route_stop(
    TradeRoute const& route, int const stop ) {
  CHECK_GE( stop, 0 );
  if( stop >= ssize( route.stops ) ) return nothing;
  return route.stops[stop];
}

maybe<TradeRouteStop const&> look_up_next_trade_route_stop(
    TradeRoute const& route, int const curr_stop ) {
  int next = curr_stop;
  ++next;
  if( next >= ssize( route.stops ) ) next = 0;
  return look_up_trade_route_stop( route, next );
}

maybe<TradeRouteTarget const&> curr_trade_route_target(
    TradeRouteState const& trade_routes, Unit const& unit ) {
  auto const trade_orders =
      unit.orders().get_if<unit_orders::trade_route>();
  if( !trade_orders.has_value() ) return nothing;
  auto const route_iter =
      trade_routes.routes.find( trade_orders->id );
  if( route_iter == trade_routes.routes.end() ) return nothing;
  TradeRoute const& route = route_iter->second;
  auto const stop         = look_up_trade_route_stop(
      route, trade_orders->en_route_to_stop );
  if( !stop.has_value() ) return nothing;
  return stop->target;
}

bool are_all_stops_identical( TradeRoute const& route ) {
  set<TradeRouteTarget> targets;
  for( TradeRouteStop const& stop : route.stops )
    targets.insert( stop.target );
  return targets.size() <= 1;
}

goto_target convert_trade_route_target_to_goto_target(
    SSConst const& ss, Player const&,
    TradeRouteTarget const& trade_route_target,
    TradeRoutesSanitizedToken const& token ) {
  validate_token( token );
  SWITCH( trade_route_target ) {
    CASE( colony ) {
      // Although colonies that are on trade routes can be
      // deleted or destroyed, this should not fail because this
      // function requires the sanitization token.
      return goto_target::map{
        .tile =
            ss.colonies.colony_for( colony.colony_id ).location,
        // We don't need a snapshot here because trade route tar-
        // gets are always friendly tiles (or the harbor) thus
        // there's no risk of ending up on a tile that we don't
        // expect. Even in the case that a unit is on the way to
        // a colony and that colony is captured, said capture
        // will be detected and the stop will be sanitized away
        // before we even get here.
        .snapshot = nothing };
    }
    CASE( harbor ) { return goto_target::harbor{}; }
  }
}

/****************************************************************
** Unit Assignments.
*****************************************************************/
bool unit_can_start_trade_route( e_unit_type const type ) {
  return unit_attr( type ).cargo_slots > 0;
}

bool unit_has_reached_trade_route_stop( SSConst const& ss,
                                        Unit const& unit ) {
  auto const trade_route_target =
      curr_trade_route_target( ss.trade_routes, unit );
  if( !trade_route_target.has_value() ) return false;
  SWITCH( *trade_route_target ) {
    CASE( colony ) {
      auto const unit_tile =
          coord_for_unit_indirect( ss.units, unit.id() );
      if( !unit_tile.has_value() ) return false;
      if( !ss.colonies.exists( colony.colony_id ) ) return false;
      point const colony_tile =
          ss.colonies.colony_for( colony.colony_id ).location;
      return *unit_tile == colony_tile;
    }
    CASE( harbor ) {
      return is_unit_in_port( ss.units, unit.id() );
    }
  }
}

/****************************************************************
** Create/Edit/Delete trade routes.
*****************************************************************/
wait<maybe<TradeRouteId>> ask_edit_trade_route(
    SSConst const& ss, Player const& player, IGui& gui ) {
  co_return co_await ask_select_trade_route(
      ss, player, gui, "Select Trade Route to Edit" );
}

wait<maybe<TradeRouteId>> ask_delete_trade_route(
    SSConst const& ss, Player const& player, IGui& gui ) {
  co_return co_await ask_select_trade_route(
      ss, player, gui, "Select Trade Route to Delete" );
}

wait<maybe<CreateTradeRoute>> ask_create_trade_route(
    SSConst const& ss, Player const& player, IGui& gui,
    TerrainConnectivity const& connectivity ) {
  vector<ColonyId> const colonies_all =
      ss.colonies.for_player_sorted( player.type );
  if( colonies_all.empty() ) {
    co_await gui.message_box(
        "We must have at least one colony to create a trade "
        "route." );
    co_return nothing;
  }
  ChoiceConfig config;
  config.msg = "Select First Stop:";
  for( ColonyId const colony_id : colonies_all ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    config.options.push_back(
        ChoiceConfigOption{ .key = to_string( colony_id ),
                            .display_name = colony.name } );
  }
  CHECK( !config.options.empty() );
  auto const first_colony_id =
      co_await gui.optional_choice_int_key( config );
  if( !first_colony_id.has_value() ) co_return nothing;
  TradeRouteTarget const stop1_target =
      TradeRouteTarget::colony{ .colony_id = *first_colony_id };
  Colony const& first_colony =
      ss.colonies.colony_for( *first_colony_id );
  using enum e_trade_route_type;
  e_trade_route_type type = land;
  if( colony_has_ocean_access( ss, connectivity,
                               first_colony.location ) ) {
    EnumChoiceConfig const enum_config{
      .msg = "What kind of trade route will this be?" };
    auto const maybe_type =
        co_await gui.optional_enum_choice<e_trade_route_type>(
            enum_config );
    if( !maybe_type.has_value() ) co_return nothing;
    type = *maybe_type;
  }
  vector<ColonyId> const second_colony_candidates = [&] {
    TradeRoute dummy;
    dummy.type = type;
    dummy.stops.push_back(
        TradeRouteStop{ .target = TradeRouteTarget::colony{
                          .colony_id = *first_colony_id } } );
    return available_colonies_for_route( ss, player,
                                         connectivity, dummy );
  }();
  // At the very least, the first colony should be in this list.
  CHECK( !second_colony_candidates.empty() );
  config     = {};
  config.msg = "Select Second Stop:";
  if( type == sea ) {
    e_nation const nation = nation_for( player.type );
    // Zero will never be a valid colony ID.
    config.options.push_back( ChoiceConfigOption{
      .key          = to_string( 0 ),
      .display_name = format(
          "Europe ({})",
          config_nation.nations[nation].harbor_city_name ) } );
  }
  for( ColonyId const colony_id : second_colony_candidates ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    config.options.push_back(
        ChoiceConfigOption{ .key = to_string( colony_id ),
                            .display_name = colony.name } );
  }
  CHECK( !config.options.empty() );
  auto const second_stop_id =
      co_await gui.optional_choice_int_key( config );
  if( !second_stop_id.has_value() ) co_return nothing;
  auto const stop2_target = [&] -> TradeRouteTarget {
    if( *second_stop_id == 0 )
      return TradeRouteTarget::harbor{};
    else
      return TradeRouteTarget::colony{ .colony_id =
                                           *second_stop_id };
  }();

  string const suggestion = trade_route_name_suggestion(
      ss.trade_routes, first_colony.name );

  string name;
  while( name.empty() ) {
    auto const candidate =
        co_await gui.optional_string_input( StringInputConfig{
          .msg          = "Choose a name for this trade route:",
          .initial_text = suggestion,
        } );
    if( !candidate.has_value() ) co_return nothing;
    if( trade_route_name_exists( ss.trade_routes,
                                 *candidate ) ) {
      co_await gui.message_box(
          "There is already a trade route with the name [{}]. "
          "Please enter a new name.",
          *candidate );
      continue;
    }
    name = *candidate;
  }

  co_return CreateTradeRoute{ .name   = name,
                              .type   = type,
                              .player = player.type,
                              .stop1  = stop1_target,
                              .stop2  = stop2_target };
}

TradeRoute create_trade_route_object(
    TradeRouteState& trade_routes,
    CreateTradeRoute const& params ) {
  TradeRouteId const trade_route_id =
      ++trade_routes.last_trade_route_id;
  return TradeRoute{
    .id     = trade_route_id,
    .name   = params.name,
    .player = params.player,
    .type   = params.type,
    .stops  = { TradeRouteStop{ .target = params.stop1 },
                TradeRouteStop{ .target = params.stop2 } } };
}

wait<base::NoDiscard<bool>> confirm_delete_trade_route(
    SSConst const& ss, IGui& gui,
    TradeRouteId const trade_route_id ) {
  auto const iter =
      ss.trade_routes.routes.find( trade_route_id );
  CHECK( iter != ss.trade_routes.routes.end() );
  TradeRoute const& route = iter->second;
  string const msg        = format(
      "Are you sure that you want to delete the [{}] trade "
             "route?",
      route.name );
  auto const ok = co_await gui.optional_yes_no( YesNoConfig{
    .msg            = msg,
    .yes_label      = "Yes",
    .no_label       = "No",
    .no_comes_first = true,
  } );
  co_return ok == ui::e_confirm::yes;
}

void delete_trade_route( TradeRouteState& trade_routes,
                         TradeRouteId trade_route_id ) {
  trade_routes.routes.erase( trade_route_id );
}

vector<ColonyId> available_colonies_for_route(
    SSConst const& ss, Player const& player,
    TerrainConnectivity const& connectivity,
    TradeRoute const& route ) {
  using enum e_trade_route_type;
  switch( route.type ) {
    case land: {
      // Find the first colony as a reference point for terrain
      // connectivity. If there is none, then just admit all
      // colonies.
      auto const first_colony_id = [&] -> maybe<ColonyId> {
        for( TradeRouteStop const& stop : route.stops )
          if( auto const colony_id =
                  stop.target
                      .inner_if<TradeRouteTarget::colony>();
              colony_id.has_value() )
            return *colony_id;
        return nothing;
      }();
      if( !first_colony_id.has_value() )
        return ss.colonies.for_player_sorted( player.type );
      Colony const& first_colony =
          ss.colonies.colony_for( *first_colony_id );
      return find_connected_colonies(
          ss, connectivity, player.type, first_colony.location );
    }
    case sea:
      // Technically this is not perfect because not all colonies
      // with sea lane access necessarily need to be connected to
      // eachother via water, especially on modded maps. But this
      // should be good enough, because if the player chooses
      // colonies that are not connected, the unit will eventu-
      // ally notice and refuse to go. Doing this properly would
      // be a pain because you'd have to check all tiles around
      // the first colony to see which regions it is connected
      // to, then find only colonies that have surrounding tiles
      // connected to one of those. And even that would not be
      // perfect because you could have a map where two bodies of
      // water (both with sea lane access) are only connected via
      // a land tile containing a colony, which would cause
      // pathing issues. Just doing the full list of coastal
      // colonies should be sufficient for almost all cases in
      // practice and shouldn't cause harm even when not perfect.
      return find_coastal_colonies( ss, connectivity,
                                    player.type );
  }
}

string name_for_target(
    SSConst const& ss, Player const& player,
    TradeRouteTarget const& target,
    TradeRoutesSanitizedToken const& token ) {
  validate_token( token );
  string name;
  SWITCH( target ) {
    CASE( colony ) {
      name = ss.colonies.colony_for( colony.colony_id ).name;
      break;
    }
    CASE( harbor ) {
      name =
          config_nation.nations[player.nation].harbor_city_name;
      break;
    }
  }
  return name;
}

vector<TradeRouteId> find_eligible_trade_routes_for_unit(
    SSConst const& ss, Unit const& unit ) {
  vector<TradeRouteId> res;
  for( auto const& [id, route] : ss.trade_routes.routes ) {
    if( route.player != unit.player_type() ) continue;
    if( route.type != trade_route_type_for_unit( unit ) )
      continue;
    // NOTE: we don't check the number of stops here, that will
    // be checked later.
    res.push_back( id );
  }
  return res;
}

wait<maybe<TradeRouteId>> select_trade_route(
    SSConst const& ss, Unit const& unit, IGui& gui,
    vector<TradeRouteId> const& route_ids ) {
  maybe<TradeRouteId> res;
  ChoiceConfig config{
    .msg = format( "Select Trade Route for [{}]:",
                   unit.desc().name ) };
  for( TradeRouteId const id : route_ids ) {
    auto const iter = ss.trade_routes.routes.find( id );
    CHECK( iter != ss.trade_routes.routes.end(),
           "trade route {} does not exist", id );
    TradeRoute const& route = iter->second;
    config.options.push_back( ChoiceConfigOption{
      .key = to_string( id ), .display_name = route.name } );
  }
  if( !config.options.empty() )
    res = co_await gui.optional_choice_int_key( config );
  co_return res;
}

wait<maybe<int>> ask_first_stop(
    SSConst const& ss, Player const& player, IGui& gui,
    TradeRouteId const route_id,
    TradeRoutesSanitizedToken const& token ) {
  maybe<int> res;
  validate_token( token );
  auto const iter = ss.trade_routes.routes.find( route_id );
  CHECK( iter != ss.trade_routes.routes.end(),
         "trade route {} does not exist", route_id );
  TradeRoute const& route = iter->second;
  ChoiceConfig config{ .msg = "Select initial destination:" };
  for( int idx = 0; TradeRouteStop const& stop : route.stops )
    config.options.push_back( ChoiceConfigOption{
      .key = to_string( idx++ ),
      .display_name =
          name_for_target( ss, player, stop.target, token ) } );
  if( !config.options.empty() )
    res = co_await gui.optional_choice_int_key( config );
  else
    // Should not happen because we sanitized, but let's just be
    // defensive.
    co_await gui.message_box(
        "Trade Route [{}] has no stops registered!",
        route.name );
  co_return res;
}

wait<maybe<TradeRouteOrdersConfirmed>>
confirm_trade_route_orders(
    SSConst const& ss, Player const& player, Unit const& unit,
    IGui& gui, TradeRoutesSanitizedToken const& token ) {
  validate_token( token );
  if( !unit_can_start_trade_route( unit.type() ) ) {
    // Not expected to happen since the land-view should not
    // allow trade route orders to be assigned to this unit,
    // but just in case...
    co_await gui.message_box(
        "[{}] cannot carry out trade routes.",
        unit.desc().name_plural );
    co_return nothing;
  }

  if( ss.trade_routes.routes.empty() ) {
    co_await gui.message_box(
        "We have not yet defined any trade routes." );
    co_return nothing;
  }

  vector<TradeRouteId> const routes =
      find_eligible_trade_routes_for_unit( ss, unit );
  if( routes.empty() ) {
    co_await gui.message_box(
        "Our [{}] is not eligible for any of the trade routes "
        "that we have defined.",
        unit.desc().name );
    co_return nothing;
  }

  auto const route_id =
      co_await select_trade_route( ss, unit, gui, routes );
  if( !route_id.has_value() )
    // Cancelled by the player.
    co_return nothing;

  TradeRouteId const trade_route_id = *route_id;

  auto const first_stop = co_await ask_first_stop(
      ss, player, gui, trade_route_id, token );
  if( !first_stop.has_value() )
    // Either this route has no stops or the player cancelled.
    co_return nothing;

  co_return TradeRouteOrdersConfirmed{
    .id = trade_route_id, .en_route_to_stop = *first_stop };
}

/****************************************************************
** Load/Unload.
*****************************************************************/
void trade_route_unload( SS& ss, Player& player, Unit& unit,
                         TradeRouteStop const& stop ) {
  bool const is_harbor =
      stop.target.holds<TradeRouteTarget::harbor>();
  auto const has_boycott = [&]( e_commodity const comm ) {
    return ss.players.old_world[player.nation]
        .market.commodities[comm]
        .boycott;
  };

  enum_map<e_commodity, int> unloaded;
  for( e_commodity const type : stop.unloads ) {
    for( auto const& [comm, slot] :
         unit.cargo().commodities( type ) ) {
      if( is_harbor && has_boycott( comm.type ) ) continue;
      Commodity const removed = rm_commodity_from_cargo(
          ss.units, unit.cargo(), slot );
      CHECK_EQ( removed, comm );
      unloaded[removed.type] += removed.quantity;
    }
  }

  SWITCH( stop.target ) {
    CASE( colony ) {
      Colony& colony_o =
          ss.colonies.colony_for( colony.colony_id );
      CHECK_EQ( colony_o.player, unit.player_type() );
      CHECK_EQ( ss.units.coord_for( unit.id() ),
                colony_o.location );
      for( auto const [comm, q] : unloaded )
        colony_o.commodities[comm] += q;
      break;
    }
    CASE( harbor ) {
      CHECK( is_unit_in_port( ss.units, unit.id() ) );
      for( auto const [comm, q] : unloaded ) {
        if( q == 0 ) continue;
        // Should have been checked above.
        CHECK( !has_boycott( comm ) );
        Invoice const invoice = transaction_invoice(
            ss, player, Commodity{ .type = comm, .quantity = q },
            e_transaction::sell,
            e_immediate_price_change_allowed::allowed );
        apply_invoice( ss, player, invoice );
      }
      break;
    }
  }
}

static void trade_route_load_colony(
    SS& ss, Player& player, Unit& unit, Colony& colony,
    vector<e_commodity> const& desired ) {
  unit.cargo().compactify( ss.as_const.units );

  while( true ) {
    vector<Commodity> const loadables =
        colony_commodities_by_value_restricted(
            ss.as_const, as_const( player ), as_const( colony ),
            desired );
    bool loaded_something = false;
    for( Commodity const& comm : loadables ) {
      int const available         = comm.quantity;
      int const try_load_quantity = std::min(
          { available, 100,
            unit.cargo().max_commodity_quantity_that_fits(
                comm.type ) } );
      CHECK_GE( try_load_quantity, 0 );
      if( try_load_quantity == 0 ) continue;
      colony.commodities[comm.type] -= try_load_quantity;
      CHECK_GE( colony.commodities[comm.type], 0 );
      // This will check-fail if the commodity does not fit, but
      // that should not happen because we should have checked
      // that max quantity that can fit already above.
      add_commodity_to_cargo(
          ss.as_const.units,
          with_quantity( comm, try_load_quantity ), unit.cargo(),
          /*slot=*/0,
          /*try_other_slots=*/true );
      loaded_something = true;
      break;
    }
    if( !loaded_something ) break;
  }
}

static void trade_route_load_harbor(
    SS& ss, Player& player, Unit& unit,
    vector<e_commodity> const& desired ) {
  auto const has_boycott = [&]( e_commodity const comm ) {
    return ss.players.old_world[player.nation]
        .market.commodities[comm]
        .boycott;
  };

  CHECK( is_unit_in_port( ss.units, unit.id() ) );
  for( e_commodity const type : desired ) {
    if( has_boycott( type ) ) continue;
    int const q = std::min(
        unit.cargo().max_commodity_quantity_that_fits( type ),
        100 );
    if( q == 0 ) continue;
    Commodity const comm{ .type = type, .quantity = q };
    Invoice const invoice = transaction_invoice(
        ss.as_const, as_const( player ), comm,
        e_transaction::buy,
        e_immediate_price_change_allowed::allowed );
    CHECK_LE( invoice.money_delta_final, 0 );
    if( -invoice.money_delta_final > player.money )
      // Cannot afford.
      continue;
    apply_invoice( ss, player, invoice );
    add_commodity_to_cargo( ss.as_const.units, comm,
                            unit.cargo(),
                            /*slot=*/0,
                            /*try_other_slots=*/true );
  }
}

void trade_route_load( SS& ss, Player& player, Unit& unit,
                       TradeRouteStop const& stop ) {
  SWITCH( stop.target ) {
    CASE( colony ) {
      trade_route_load_colony(
          ss, player, unit,
          ss.colonies.colony_for( colony.colony_id ),
          stop.loads );
      break;
    }
    CASE( harbor ) {
      trade_route_load_harbor( ss, player, unit, stop.loads );
      break;
    }
  }
}

/****************************************************************
** End to end.
*****************************************************************/
EvolveTradeRoute evolve_trade_route_human(
    SS& ss, Player& player, GotoRegistry& goto_registry,
    TerrainConnectivity const& connectivity, Unit& unit ) {
  auto const abort = [&]( string const& msg ) {
    lg.debug( "aborting trade route: {}", msg );
    goto_registry.units.erase( unit.id() );
    return EvolveTradeRoute::abort{};
  };

  if( unit.has_full_mv_points() &&
      goto_registry.units.contains( unit.id() ) ) {
    // If we're at the start of a unit's turn and the path has
    // already been computed and it is short enough then it
    // should be recomputed (once, at the start of the turn).
    // That way, if e.g. a wagon train planned a course while
    // there was an enemy unit blocking a road and that unit has
    // since moved then it will not go off-road (around the for-
    // eign unit) for no reason; it will take the road. This will
    // probably happen often due to braves getting in the way.
    if( goto_registry.units[unit.id()].path.reverse_path.size() <
        20 )
      goto_registry.units.erase( unit.id() );
  }

  // We can't really show a message here communicating the ac-
  // tions taken, but hopefully it won't matter because sanitiza-
  // tion will be done interactively just before calling the
  // method we are currently in.
  vector<TradeRouteSanitizationAction> actions_taken;
  TradeRoutesSanitizedToken const& token =
      sanitize_trade_routes( ss.as_const, as_const( player ),
                             ss.trade_routes, actions_taken );
  CHECK( actions_taken.empty(),
         "trade routes must be sanitized before calling" );
  auto const sanitized_orders = sanitize_unit_trade_route_orders(
      ss.as_const, as_const( unit ), token );
  if( !sanitized_orders.has_value() )
    return abort( "unit trade route orders no longer valid" );
  unit_orders::trade_route& orders =
      unit.orders().emplace<unit_orders::trade_route>();
  orders = *sanitized_orders;

  // All of these checks should have been checked in the sanitize
  // unit orders step above.
  UNWRAP_CHECK_T(
      TradeRoute const& route,
      look_up_trade_route( ss.trade_routes, orders.id ) );
  UNWRAP_CHECK_T( TradeRouteStop const& stop,
                  look_up_trade_route_stop(
                      route, orders.en_route_to_stop ) );

  goto_target const target_goto =
      convert_trade_route_target_to_goto_target(
          ss, as_const( player ), stop.target, token );

  if( !unit_has_reached_goto_target( ss, as_const( unit ),
                                     target_goto ) ) {
    if( is_unit_in_port( ss.units, unit.id() ) )
      return EvolveTradeRoute::sail_to_new_world{};
    SWITCH( evolve_goto_human( ss, connectivity, goto_registry,
                               unit, target_goto ) ) {
      CASE( abort ) { return EvolveTradeRoute::abort_no_path{}; }
      CASE( move ) {
        return EvolveTradeRoute::move{ .to = move.to };
      }
    }
  }

  // At this point we have reached our current goto target, so we
  // must do the unload/load.
  trade_route_unload( ss, player, unit, stop );
  trade_route_load( ss, player, unit, stop );

  if( ++orders.en_route_to_stop >= ssize( route.stops ) )
    orders.en_route_to_stop = 0;

  // This will catch the case where there is only one unique stop
  // on the route. This will prevent infinite loops which would
  // otherwise happen since we normally process consecutive stops
  // all in one shot when they have the same target.
  if( are_all_stops_identical( route ) )
    return EvolveTradeRoute::one_unique_stop{};

  // Recurse.
  return evolve_trade_route_human( ss, player, goto_registry,
                                   connectivity, unit );
}

} // namespace rn
