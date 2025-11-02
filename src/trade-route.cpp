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
#include "colony-mgr.hpp"
#include "connectivity.hpp"
#include "igui.hpp"
#include "trade-route-ui.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/trade.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/nation.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/trade-route.hpp"

using namespace std;

namespace rn {

namespace {

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
** Public API.
*****************************************************************/
[[nodiscard]] bool unit_can_start_trade_route(
    e_unit_type const type ) {
  return unit_attr( type ).cargo_slots > 0;
}

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

} // namespace rn
