/****************************************************************
**tribe.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-30.
*
* Description: Represents one indian tribe.
*
*****************************************************************/
#include "tribe.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** TribeRelationship
*****************************************************************/
base::valid_or<string> TribeRelationship::validate() const {
  REFL_VALIDATE( tribal_alarm >= 0 && tribal_alarm <= 99,
                 "tribal_alarm must be in [0, 99]." );
  return base::valid;
}

} // namespace rn
