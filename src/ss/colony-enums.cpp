/****************************************************************
**colony-enums.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-12.
*
* Description: Colony-related enums.
*
*****************************************************************/
#include "colony-enums.hpp"

using namespace std;

namespace rn {

e_outdoor_job to_outdoor_job(
    e_outdoor_commons_secondary_job job ) {
  switch( job ) {
    case e_outdoor_commons_secondary_job::sugar:
      return e_outdoor_job::sugar;
    case e_outdoor_commons_secondary_job::tobacco:
      return e_outdoor_job::tobacco;
    case e_outdoor_commons_secondary_job::cotton:
      return e_outdoor_job::cotton;
    case e_outdoor_commons_secondary_job::fur:
      return e_outdoor_job::fur;
    case e_outdoor_commons_secondary_job::ore:
      return e_outdoor_job::ore;
  }
}

} // namespace rn
