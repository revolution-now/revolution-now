/****************************************************************
**colony.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-15.
*
* Description: Data structure representing a Colony.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "colony-structures.hpp"
#include "commodity.hpp"
#include "enum.hpp"
#include "errors.hpp"
#include "id.hpp"
#include "nation.hpp"

// base-util
#include "base-util/non-copyable.hpp"

namespace rn {

enum class e_( job_in_colony,
               // Land
               land_nw, //
               land_n,  //
               land_ne, //
               land_w,  //
               land_e,  //
               land_sw, //
               land_s,  //
               land_se, //

               // In colony.
               coats //
);

class Colony : public util::movable_only {
public:
  expect<> check_invariants_safe() const;

private:
  ColonyId                         id_;
  e_nation                         nation_;
  std::string                      name_;
  FlatMap<e_commodity, int>        commodities_;
  FlatMap<UnitId, e_job_in_colony> jobs_;
  int                              prod_hammers_;
  int                              prod_tools_;
  FlatSet<e_colony_building>       buildings_;
  Opt<e_colony_building>           production_;
};

} // namespace rn
