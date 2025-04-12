/****************************************************************
**revolution.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-12.
*
* Description: Config data for the revolution module.
*
*****************************************************************/
#include "revolution.hpp"

// refl
#include "refl/ext.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

}

/****************************************************************
** config::revolution::Declaration
*****************************************************************/
base::valid_or<string>
config::revolution::Declaration::validate() const {
  for( auto const& [difficulty, rebels] :
       ai_required_number_of_rebels )
    REFL_VALIDATE(
        rebels > 0,
        "Required number of rebels for AI to be granted "
        "independence must be larger than zero." );

  return base::valid;
}

} // namespace rn
