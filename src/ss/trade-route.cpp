/****************************************************************
**trade-route.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-25.
*
* Description: Save-game state for trade routes.
*
*****************************************************************/
#include "trade-route.hpp"

// refl
#include "refl/ext.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

}

/****************************************************************
** TradeRouteTarget
*****************************************************************/
valid_or<string> TradeRouteTarget::harbor::validate() const {
  return valid;
}

valid_or<string> TradeRouteTarget::colony::validate() const {
  REFL_VALIDATE( colony_id > 0,
                 "invalid colony ID in trade route target: {}",
                 colony_id );
  return valid;
}

/****************************************************************
** TradeRoute
*****************************************************************/
valid_or<string> TradeRoute::validate() const {
  REFL_VALIDATE(
      id > 0, "invalid trade route ID in trade route target: {}",
      id );

  REFL_VALIDATE( stops.size() <= 4,
                 "trade route {} has {} stops but trade routes "
                 "are only allowed a maximum of four.",
                 id, stops.size() );

  // If trade route type is land then we cannot have any harbor
  // stops.
  if( type == e_trade_route_type::land )
    for( TradeRouteStop const& stop : stops )
      REFL_VALIDATE(
          !stop.target.holds<TradeRouteTarget::harbor>(),
          "A land trade route cannot have the European Harbor "
          "as a stop." );

  return valid;
}

/****************************************************************
** TradeRouteState
*****************************************************************/
valid_or<string> TradeRouteState::validate() const {
  // Must be a valid ID or zero.
  REFL_VALIDATE( prev_trade_route_id >= 0,
                 "prev_trade_route_id must be >= zero." );

  // Zero is not a valid trade route ID.
  REFL_VALIDATE( !routes.contains( TradeRouteId{ 0 } ),
                 "trade routes list contains a trade route ID "
                 "of zero which is invalid." );

  // Given that prev_trade_route_id gives the number of trade
  // routes that have been created thus far in the game, it fol-
  // lows that the current trade route count must be less or
  // equal to it.
  REFL_VALIDATE(
      ssize( routes ) <= prev_trade_route_id,
      "prev_trade_route_id indicates that there should be at "
      "most {} active trade routes, but there are {}.",
      prev_trade_route_id, routes.size() );

  // Trade route ID matches that inside route object.
  for( auto const& [id, route] : routes )
    REFL_VALIDATE(
        route.id == id,
        "inconsistent trade route IDs found: {} != {}", route.id,
        id );

  // Each active trade route ID is <= to prev_trade_route_id.
  for( auto const& [id, route] : routes )
    REFL_VALIDATE( id <= prev_trade_route_id,
                   "there is an active trade route with id={} "
                   "which is > than prev_trade_route_id={}",
                   route.id, prev_trade_route_id );

  // Each trade route must have at least one stop.
  for( auto const& [id, route] : routes )
    REFL_VALIDATE( !route.stops.empty(),
                   "trade route {} has no stops", id );

  return valid;
}

} // namespace rn
