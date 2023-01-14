/****************************************************************
**price-group.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-16.
*
* Description: Implementation of the OG's market model governing
*              the movement of prices of rum, cigars, cloth, and
*              coats.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "price-group.rds.hpp"

// Revolution Now
#include "maybe.hpp"

// ss
#include "ss/commodity.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

struct TS;

/****************************************************************
** e_processed_good
*****************************************************************/
e_commodity to_commodity( e_processed_good good );

maybe<e_processed_good> from_commodity( e_commodity comm );

/****************************************************************
** ProcessedGoodsPriceGroupConfig
*****************************************************************/
ProcessedGoodsPriceGroupConfig
default_processed_goods_price_group_config();

/****************************************************************
** ProcessedGoodsPriceGroup
*****************************************************************/
struct ProcessedGoodsPriceGroup {
  using Map = refl::enum_map<e_processed_good, int>;

  ProcessedGoodsPriceGroup(
      ProcessedGoodsPriceGroupConfig const& config );

  // Player buys.
  void buy( e_processed_good good, int quantity );

  // Player sells.
  void sell( e_processed_good good, int quantity );

  // Note that the price group model itself does not have a no-
  // tion of bid vs. ask price; it just deals with one price
  // whose interpretation is decided by the caller.
  refl::enum_map<e_processed_good, int> equilibrium_prices();

  void evolve();

  void evolve( e_processed_good good );

  int intrinsic_volume( e_processed_good good ) const {
    return intrinsic_volumes_[good];
  }

  int traded_volume( e_processed_good good ) const {
    return traded_volumes_[good];
  }

  auto const& intrinsic_volumes() const {
    return intrinsic_volumes_;
  }

  auto const& traded_volumes() const { return traded_volumes_; }

  friend void to_str( ProcessedGoodsPriceGroup const& o,
                      std::string& out, base::ADL_t );

 private:
  void transaction( e_processed_good good, int quantity );

  void evolve_intrinsic_volume( e_processed_good good );

  ProcessedGoodsPriceGroupConfig const  config_;
  refl::enum_map<e_processed_good, int> intrinsic_volumes_;
  refl::enum_map<e_processed_good, int> traded_volumes_;
};

/****************************************************************
** Public API
*****************************************************************/
int generate_random_intrinsic_volume( TS& ts, int center,
                                      int window );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::ProcessedGoodsPriceGroup,
                     owned_by_lua ){};

} // namespace lua
