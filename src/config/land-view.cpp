/****************************************************************
**land-view.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-19.
*
* Description: Config info for the land-view module.
*
*****************************************************************/
#include "land-view.rds.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

}

/****************************************************************
** config::revolution::Declaration
*****************************************************************/
valid_or<string> config::land_view::Camera::validate() const {
  REFL_VALIDATE( zoom_log2_min <= zoom_log2_max,
                 "zoom_log2_min must be <= zoom_log2_max" );

  return base::valid;
}

} // namespace rn
