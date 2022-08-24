/****************************************************************
**market.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-21.
*
* Description: Manipulates/evolves market prices.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "market.rds.hpp"

// ss
#include "ss/commodity.rds.hpp"

namespace rn {

struct Player;

/****************************************************************
** Public API
*****************************************************************/
CommodityPrice market_price( Player const& player,
                             e_commodity   commodity );

} // namespace rn
