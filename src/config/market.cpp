/****************************************************************
**market.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-21.
*
# Description: Config info for market prices.
*
*****************************************************************/
#include "market.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

base::valid_or<string> config::market::PriceLimits::validate()
    const {
  REFL_VALIDATE(
      bid_price_start_min <= bid_price_start_max,
      "bid_price_start_min must be <= bid_price_start_max" );
  REFL_VALIDATE( bid_price_min <= bid_price_max,
                 "bid_price_min must be <= bid_price_max" );
  REFL_VALIDATE( bid_price_start_min >= 0,
                 "bid_price_start_min must be >= 0." );
  REFL_VALIDATE( bid_price_start_max <= 19,
                 "bid_price_start_max must be <= 19." );
  // The original game always has at least 1 for bid/ask spread,
  // but we'll allow it to be zero in case someone wants to mod
  // the game in that way.
  REFL_VALIDATE( bid_ask_spread >= 0,
                 "bid_bid_spread must be >= 0." );
  REFL_VALIDATE(
      bid_price_start_max + bid_ask_spread <= 20,
      "bid_price_start_max + bid_ask_spread must be <= 20" );
  REFL_VALIDATE(
      bid_price_max + bid_ask_spread <= 20,
      "bid_price_max + bid_ask_spread must be <= 20" );
  return base::valid;
}

base::valid_or<string> config::market::PriceGroup::validate()
    const {
  REFL_VALIDATE( bid_price_min <= bid_price_max,
                 "bid_price_min must be <= bid_price_max" );
  return base::valid;
}

base::valid_or<string> config::market::EconomicModel::validate()
    const {
  REFL_VALIDATE( rise > 0, "rise must be > 0." );
  REFL_VALIDATE( fall > 0, "fall must be > 0." );
  REFL_VALIDATE( volatility < 8, "volatility < 8" );
  return base::valid;
}

base::valid_or<string>
config::market::NationAdvantage::validate() const {
  REFL_VALIDATE(
      attrition_scale >= 1.0,
      "the attrition scaling for the dutch must be >= 1.0." );
  REFL_VALIDATE(
      sell_volume_scale <= 1.0,
      "the sell volume scaling for the dutch must be <= 1.0." );
  return base::valid;
}

} // namespace rn
