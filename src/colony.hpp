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
#include "adt.hpp"
#include "colony-structures.hpp"
#include "commodity.hpp"
#include "enum.hpp"
#include "errors.hpp"
#include "id.hpp"
#include "nation.hpp"

// base-util
#include "base-util/non-copyable.hpp"

namespace rn {

adt_rn( ColonyJob,                            //
        ( land,                               //
          ( e_direction, d ) ),               //
        ( building,                           //
          ( e_inside_colony_job, building ) ) //
);

class Colony : public util::movable_only {
public:
  expect<> check_invariants_safe() const;

private:
  // Basic info.
  ColonyId    id_;
  e_nation    nation_;
  std::string name_;

  // Commodities.
  FlatMap<e_commodity, int> commodities_;

  // Serves to both record the units in this colony as well as
  // their occupations.
  FlatMap<UnitId, ColonyJob_t> jobs_;
  FlatSet<e_colony_building>   buildings_;

  // Production
  Opt<e_colony_building> production_;
  int                    prod_hammers_;
  int                    prod_tools_;

  // Liberty sentiment: [0,100].
  int sentiment_;
};

} // namespace rn
