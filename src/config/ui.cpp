/****************************************************************
**ui.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-14.
*
# Description: Config info for UI stuff.
*
*****************************************************************/
#include "ui.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

base::valid_or<string> config::ui::console::validate() const {
  REFL_VALIDATE( size_percentage >= 0.0,
                 "size_percentage must be >= 0.0" );
  REFL_VALIDATE( size_percentage <= 1.0,
                 "size_percentage must be <= 1.0" );
  return base::valid;
}

} // namespace rn
