/****************************************************************
**user.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-17.
*
* Description: User-level config data.
*
*****************************************************************/
#include "user.rds.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** config_user_t
*****************************************************************/
valid_or<string> config_user_t::validate() const {
  // None yet.
  return valid;
}

} // namespace rn
