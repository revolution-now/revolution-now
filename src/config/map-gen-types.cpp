/****************************************************************
**map-gen-types.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-03.
*
* Description: Config data for the map-gen-types module.
*
*****************************************************************/
#include "map-gen-types.rds.hpp"

// refl
#include "refl/ext.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** PerlinLandForm
*****************************************************************/
valid_or<string> PerlinLandForm::validate() const {
  return valid;
}

/****************************************************************
** PerlinEdgeSuppression
*****************************************************************/
valid_or<string> PerlinEdgeSuppression::validate() const {
  return valid;
}

} // namespace rn
