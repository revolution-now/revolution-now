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
      ask_price_start_min <= ask_price_start_max,
      "ask_price_start_min must be <= ask_price_start_max" );
  REFL_VALIDATE( ask_price_start_min > 0,
                 "ask_price_start_min must be > 0." );
  REFL_VALIDATE( ask_price_start_max <= 20,
                 "ask_price_start_max must be <= 20." );
  REFL_VALIDATE( bid_ask_spread > 0,
                 "bid_ask_spread must be > 0." );
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
