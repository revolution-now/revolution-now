/****************************************************************
**trade-route.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Handles things related to trade routes.
*
*****************************************************************/
#pragma once

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
enum class e_unit_type;

/****************************************************************
** Public API.
*****************************************************************/
[[nodiscard]] bool unit_can_start_trade_route(
    e_unit_type type );

} // namespace rn
