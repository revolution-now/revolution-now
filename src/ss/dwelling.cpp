/****************************************************************
**dwelling.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-30.
*
* Description: Represents an indian dwelling.
*
*****************************************************************/
#include "dwelling.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** DwellingRelationship
*****************************************************************/
valid_or<string> DwellingRelationship::validate() const {
  REFL_VALIDATE(
      dwelling_only_alarm >= 0 && dwelling_only_alarm <= 99,
      "dwelling_only_alarm must be in [0, 99]." );
  return valid;
}

/****************************************************************
** Dwelling
*****************************************************************/
valid_or<string> Dwelling::validate() const {
  // A dwelling object must either have an id (meaning that it is
  // a real dwelling on the map) or it must have a `frozen` rep-
  // resentation (meaning that it is a snapshot of the dwelling
  // when last visited) but cannot have both.
  bool const real_dwelling = ( id != 0 );
  bool const has_frozen    = frozen.has_value();
  REFL_VALIDATE( real_dwelling != has_frozen,
                 "Dwelling must have either a non-zero ID or a "
                 "frozen state, and not both." );

  return valid;
}

} // namespace rn
