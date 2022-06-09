/****************************************************************
**menu.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-27.
*
* Description: Menu bar
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "co-combinator.hpp"

// Rds
#include "menu.rds.hpp"
#include "plane-stack.rds.hpp"

// C++ standard library
#include <string>

namespace rn {

struct Plane;
struct Planes;

/****************************************************************
** MenuPlane
*****************************************************************/
struct MenuPlane {
  MenuPlane( Planes& planes, e_plane_stack where );
  ~MenuPlane() noexcept;

  void register_handler( e_menu_item item, Plane& plane );

  void unregister_handler( e_menu_item item, Plane& plane );

 private:
  Planes&             planes_;
  e_plane_stack const where_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace rn
