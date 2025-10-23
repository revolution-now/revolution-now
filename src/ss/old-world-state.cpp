/****************************************************************
**old-world-state.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-27.
*
# Description: Per-player old world state.
*
*****************************************************************/
#include "old-world-state.hpp"

// gs
#include "ss/market.hpp"
#include "ss/unit-type.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

base::valid_or<string> ImmigrationState::validate() const {
  // Validate that all immigrants in the pool are colonists.
  for( e_unit_type type : immigrants_pool ) {
    REFL_VALIDATE( is_unit_a_colonist( type ),
                   "units in the immigrant pool must be a "
                   "colonist, but {} is not.",
                   type );
  }

  return base::valid;
}

base::valid_or<string> TaxationState::validate() const {
  REFL_VALIDATE( tax_rate >= 0, "The tax rate must be >= 0." );
  REFL_VALIDATE( tax_rate <= 100,
                 "The tax rate must be <= 100." );

  return base::valid;
}

} // namespace rn
