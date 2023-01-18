/****************************************************************
**old-world.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-05.
*
# Description: Config info for things in the Old World.
*
*****************************************************************/
#include "old-world.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

base::valid_or<string> config_old_world_t::validate() const {
  return base::valid;
}

} // namespace rn
