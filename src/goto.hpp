/****************************************************************
**goto.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-25.
*
* Description: Goto-related things.
*
*****************************************************************/
#pragma once

// rds
#include "goto.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/maybe.hpp"

namespace rn {

struct IGotoMapViewer {
  virtual ~IGotoMapViewer() = default;

  [[nodiscard]] virtual bool can_enter_tile(
      gfx::point tile ) const = 0;
};

/****************************************************************
** Public API.
*****************************************************************/
base::maybe<GotoPath> compute_goto_path(
    IGotoMapViewer const& viewer, gfx::point src,
    gfx::point dst );

} // namespace rn
