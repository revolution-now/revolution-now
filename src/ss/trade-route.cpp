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
  return valid;
}

/****************************************************************
** TradeRoute
*****************************************************************/
valid_or<string> TradeRoute::validate() const {
  return valid;
}

/****************************************************************
** TradeRouteState
*****************************************************************/
valid_or<string> TradeRouteState::validate() const {
  return valid;
}

} // namespace rn
