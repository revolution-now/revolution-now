/****************************************************************
**production.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-31.
*
* Description: Computes what is produced by a colony.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// Rds
#include "colony-buildings.rds.hpp"
#include "production.rds.hpp"

namespace rn {

struct Colony;
struct SSConst;

// Computes everything that is produced and consumed by the
// colony in one turn, given the current state of the colony, all
// things considered. Note that some items (such as tools) can be
// both produced and consumed in the same turn, and so those will
// have to be subtracted to get the net change.
ColonyProduction production_for_colony( SSConst const& ss,
                                        Colony const&  colony );

// Given a building slot, will extract the quantity of the thing
// currently being produced there.
maybe<int> production_for_slot( ColonyProduction const& pr,
                                e_colony_building_slot  slot );

// If a product can be made from this raw material, return it.
maybe<e_commodity> product_from_raw( e_commodity raw );

int const& final_production_delta_for_commodity(
    ColonyProduction const& pr, e_commodity c );

} // namespace rn
