/****************************************************************
**job.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-27.
*
* Description: Handles job orders on units.
*
*****************************************************************/
#include "job.hpp"

// base-util
#include "base-util/variant.hpp"

namespace rn {

namespace {} // namespace

bool ProposedJobAnalysisResult::allowed() const {
  return util::holds<e_unit_job_good>( desc ) != nullptr;
}

} // namespace rn
