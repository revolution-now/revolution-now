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
  REFL_VALIDATE( !routes.contains( TradeRouteId{ 0 } ),
                 "trade routes list contains a trade route ID "
                 "of zero which is invalid." );

  // Trade route ID matches that inside route object.
  for( auto const& [id, route] : routes )
    REFL_VALIDATE(
        route.id == id,
        "inconsistent trade route IDs found: {} != {}", route.id,
        id );

  return valid;
}

} // namespace rn
