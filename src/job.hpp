/****************************************************************
**job.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-27.
*
* Description: Handles job orders on units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <variant>

namespace rn {

enum class ND e_unit_job_good {};

enum class ND e_unit_job_error {};

using v_unit_job_desc =
    std::variant<e_unit_job_good, e_unit_job_error>;

struct ProposedJobAnalysisResult {
  // Is this order possible at all.
  bool allowed() const;

  // Description of what would happen if the move were carried
  // out. This can also serve as a binary indicator of whether
  // the move is possible by checking the type held, as the can_-
  // move() function does as a convenience.
  v_unit_job_desc desc{};
};

} // namespace rn
