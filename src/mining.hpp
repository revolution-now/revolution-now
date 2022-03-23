/****************************************************************
**mining.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-25.
*
* Description: Logic for computing production on a square.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "commodity.hpp"
#include "map-square.hpp"
#include "utype.hpp"

namespace rn {

enum class e_land_production_reason {
  // Land can produce some non-zero amount of commodity.
  ok,
  // Water squares can only yield fish.
  only_fish_in_water,
  // Land squares can never yield fish (even with river).
  no_fish_on_land,
  // For manufactured commodities like cloth which cannot be di-
  // rectly mined from the land.
  cannot_be_mined,
  // This is a kind of catch-all, for cases when the land type
  // simply doesn't yield the given commodity, for no other spe-
  // cific reason.
  void_of_commodity
};

struct UnitLandProductionInfo {
  // Number of good produced per turn.
  int quantity;
  // Explanation for value of quantity. In particular, if `quan-
  // tity` is zero then this holds the reason.
  e_land_production_reason reason;
};

} // namespace rn
