/****************************************************************
**options.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-08-08.
*
# Description: Options related to game rules.
*
*****************************************************************/
#include "options.rds.hpp"

// refl
#include "refl/ext.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** GameRulesOptions
*****************************************************************/
valid_or<string> GameRulesOptions::validate() const {
  return valid;
}

} // namespace rn
