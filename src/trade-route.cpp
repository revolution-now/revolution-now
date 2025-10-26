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

// config
#include "config/unit-type.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
[[nodiscard]] bool unit_can_start_trade_route(
    e_unit_type const type ) {
  return unit_attr( type ).cargo_slots > 0;
}

} // namespace rn
