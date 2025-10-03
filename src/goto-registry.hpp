/****************************************************************
**goto-registry.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-26.
*
* Description: Holds pre-computed unit go-to paths.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "goto.rds.hpp"

// ss
#include "ss/goto.rds.hpp"
#include "ss/unit-id.hpp"

namespace rn {

struct GotoExecution {
  goto_target target;
  GotoPath path;
};

struct GotoRegistry {
  std::unordered_map<UnitId, GotoExecution> paths;
};

} // namespace rn
