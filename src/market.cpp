/****************************************************************
**market.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-21.
*
* Description: Manipulates/evolves market prices.
*
*****************************************************************/
#include "market.hpp"

// config
#include "config/market.rds.hpp"

// ss
#include "ss/player.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

void linker_dont_discard_module_market();
void linker_dont_discard_module_market() {}

/****************************************************************
** Public API
*****************************************************************/
CommodityPrice market_price( Player const& player,
                             e_commodity   commodity ) {
  int const bid = player.old_world.market.commodities[commodity]
                      .bid_price_in_hundreds;
  int const ask = bid + config_market.price_behavior[commodity]
                            .price_limits.bid_ask_spread;
  return CommodityPrice{ .bid = bid, .ask = ask };
}

} // namespace rn
