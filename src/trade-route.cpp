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
#include "connectivity.hpp"
#include "harbor-units.hpp"
#include "iagent.hpp"
#include "igui.hpp"
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

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;
using ::refl::enum_map;

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
    suggestion = "Route";
    return suggestion;
  }
  auto const get_suffix = [&]( int const idx ) {
    CHECK_GE( idx, 0 );
    CHECK_LT( idx, ssize( suffixes ) );
    return suffixes[idx];
  };
  int const start = trade_routes.prev_trade_route_id;
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
                               TradeRouteTarget const& target ) {
  SWITCH( target ) {
    CASE( colony ) {
      if( !ss.colonies.exists( colony.colony_id ) )
        return TradeRouteSanitizationAction::
            colony_no_longer_exists{};
      Colony const& colony_o =
          ss.colonies.colony_for( colony.colony_id );
      if( colony_o.player != player.type )
        return TradeRouteSanitizationAction::
            colony_changed_player{ .colony_id =
                                       colony.colony_id };
      break;
    }
    CASE( harbor ) {
      if( player.revolution.status >=
          e_revolution_status::declared )
        return TradeRouteSanitizationAction::
            harbor_inaccessible{};
      break;
    }
  }
  return nothing;
}

TradeRoutesSanitizedToken const& sanitize_trade_routes(
    SSConst const& ss, Player const& player,
    TradeRouteState& trade_routes,
    vector<TradeRouteSanitizationAction>& actions_taken ) {
  static TradeRoutesSanitizedToken const kToken;
  actions_taken.clear();

  // Erase inaccessible stops.
  for( auto& [route_id, route] : trade_routes.routes ) {
    if( route.player != player.type ) continue;
    bool const needs_removing = [&] {
      for( TradeRouteStop const& stop : route.stops )
        if( trade_route_stop_inaccessible( ss, player,
                                           stop.target ) )
          return true;
      return false;
    }();
    if( !needs_removing ) continue;
    vector<TradeRouteStop> new_stops;
    new_stops.reserve( route.stops.size() );
    for( TradeRouteStop const& stop : route.stops ) {
      auto const reason = trade_route_stop_inaccessible(
          ss, player, stop.target );
      if( reason.has_value() ) {
        actions_taken.push_back( *reason );
        continue;
      }
      new_stops.push_back( stop );
    }
    route.stops = new_stops;
  }

  // Erase empty routes.
  vector<pair<int, string>> empty;
  for( auto const& [id, route] : trade_routes.routes ) {
    if( route.player != player.type ) continue;
    if( route.stops.empty() )
      empty.push_back( pair{ id, route.name } );
  }
  for( auto const& [id, name] : empty ) {
    actions_taken.push_back(
        TradeRouteSanitizationAction::empty_route{ .name =
                                                       name } );
    trade_routes.routes.erase( id );
  }

  return kToken;
}

wait<> show_sanitization_actions(
    SSConst const&, IAgent& agent,
    vector<TradeRouteSanitizationAction> const& actions_taken,
    TradeRoutesSanitizedToken const& token ) {
  validate_token( token );
  if( actions_taken.empty() ) co_return;
  for( TradeRouteSanitizationAction const& action :
       actions_taken ) {
    SWITCH( action ) {
      CASE( colony_no_longer_exists ) {
        break;
      }
      CASE( colony_changed_player ) {
        break;
      }
      CASE( harbor_inaccessible ) {
        break;
      }
      CASE( empty_route ) {
        break;
      }
    }
  }
  co_await agent.message_box(
      "Some trade routes and/or trade route stops were "
      "removed." );
  co_return;
}

wait<std::reference_wrapper<TradeRoutesSanitizedToken const>>
run_trade_route_sanitization( SSConst const& ss,
                              Player const& player,
                              TradeRouteState& trade_routes,
                              IAgent& agent ) {
  vector<TradeRouteSanitizationAction> actions_taken;
  TradeRoutesSanitizedToken const& token = sanitize_trade_routes(
      ss, player, trade_routes, actions_taken );
  co_await show_sanitization_actions(
      ss, agent, as_const( actions_taken ), token );
  co_return token;
}

maybe<unit_orders::trade_route> sanitize_unit_orders(
    SSConst const& ss, unit_orders::trade_route const& orders,
    TradeRoutesSanitizedToken const& token ) {
  maybe<unit_orders::trade_route> res;
  validate_token( token );
  auto const route =
      look_up_trade_route( ss.trade_routes, orders.id );
  if( !route.has_value() ) return res;
  // This should have been caught in the sanitization process.
  CHECK( !route->stops.empty() );
  auto& new_orders = res.emplace();
  new_orders       = orders;
  if( new_orders.en_route_to_stop >= ssize( route->stops ) )
    new_orders.en_route_to_stop = 0;
  return res;
}

/****************************************************************
** Querying.
*****************************************************************/
maybe<TradeRoute&> look_up_trade_route(
    TradeRouteState& trade_routes, TradeRouteId const id ) {
  auto const iter = trade_routes.routes.find( id );
  if( iter == trade_routes.routes.end() ) return nothing;
  return iter->second;
}

maybe<TradeRoute const&> look_up_trade_route(
    TradeRouteState const& trade_routes,
    TradeRouteId const id ) {
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
  struct comp {
    [[maybe_unused]] static bool operator()(
        TradeRouteTarget const& l, TradeRouteTarget const& r ) {
      return base::to_str( l ) < base::to_str( r );
    }
  };
  set<TradeRouteTarget, comp> targets;
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
      ++trade_routes.prev_trade_route_id;
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
      // colonies that are not connected, then unit will eventu-
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
      // practice and won't cause any harm even when it is not
      // perfect.
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
    switch( route.type ) {
      case e_trade_route_type::land: {
        if( unit.desc().ship ) continue;
        break;
      }
      case e_trade_route_type::sea: {
        if( !unit.desc().ship ) continue;
        break;
      }
    }
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
    co_await gui.message_box(
        "Trade Route [{}] has no stops registered.",
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
        "This [{}] is not eligible for any of the trade "
        "routes that we have defined.",
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

} // namespace rn
