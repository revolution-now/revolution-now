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

namespace rn {

struct Player;
struct SS;
struct TS;

/****************************************************************
** ProcessedGoodsPriceGroup
*****************************************************************/
struct ProcessedGoodsPriceGroup {
  ProcessedGoodsPriceGroup(
      SS& ss, Player& player,
      ProcessedGoodsPriceGroupConfig const& config );

  // Player buys.
  void buy( e_processed_good good, int quantity );

  // Player sells.
  void sell( e_processed_good good, int quantity );

  refl::enum_map<e_processed_good, int> equilibrium_prices();

  void evolve();

 private:
  void transaction( e_processed_good good, int quantity );

  void evolve_intrinsic_volume( e_processed_good good );

  ProcessedGoodsPriceGroupConfig const  config_;
  refl::enum_map<e_processed_good, int> intrinsic_volumes_;
  refl::enum_map<e_processed_good, int> traded_volumes_;
  SS&                                   ss_;
  Player&                               player_;
};

/****************************************************************
** Public API
*****************************************************************/
int generate_random_intrinsic_volume( TS& ts, int center,
                                      int window );

} // namespace rn
