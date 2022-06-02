/****************************************************************
**production.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-02.
*
# Description: Config info for colony production.
*
*****************************************************************/
#include "production.hpp"

using namespace std;

namespace rn {

base::valid_or<string> config_production_t::validate() const {
  return base::valid;
}

} // namespace rn
