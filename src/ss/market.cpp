/****************************************************************
**market.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-21.
*
* Description: Represents state related to european market
*              prices.
*
*****************************************************************/
#include "market.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

/****************************************************************
** PlayerMarketItem
*****************************************************************/
base::valid_or<string> PlayerMarketItem::validate() const {
  REFL_VALIDATE( bid_price >= 0, "bid_price must be >= 0" );
  REFL_VALIDATE( bid_price < 20, "bid_price must be < 20" );
  return base::valid;
}

} // namespace rn
