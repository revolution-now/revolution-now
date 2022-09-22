/****************************************************************
**harbor.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-21.
*
# Description: Config info related to the high seas and harbor
#              view.
*
*****************************************************************/
#include "harbor.hpp"

// ss
#include "ss/unit-type.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

base::valid_or<string> config::harbor::Train::validate() const {
  // FIXME: unfortunately we can't validate that the units in the
  // menu are all human (which we'd like to do) because that re-
  // quires that the unit-type config already be loaded which is
  // not guaranteed at this point since this is running while
  // configs are being loaded.
  return base::valid;
}

} // namespace rn
