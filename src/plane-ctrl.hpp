/****************************************************************
**plane-ctrl.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-26.
*
* Description: Manages ordering and enablement of planes.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace rn {

enum class e_plane_config {
  main_menu,
  land_view,
  colony,
  old_world,
  black
};

struct ScopedPlanePush {
  ScopedPlanePush( e_plane_config config );
  ~ScopedPlanePush() noexcept;
};

} // namespace rn
