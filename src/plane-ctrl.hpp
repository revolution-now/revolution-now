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
  main_menu, //
  terrain,
  colony,
  europe
};

// These do what you would think they do.
void push_plane_config( e_plane_config conf );
void pop_plane_config();

// Just after calling this you are expected to push a plane con-
// fig.
void clear_plane_stack();

} // namespace rn
