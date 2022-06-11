/****************************************************************
**console.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-03.
*
* Description: The developer/mod console.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// C++ standard library
#include <memory>

namespace rn {

struct MenuPlane;
struct Plane;

/****************************************************************
** ConsolePlane
*****************************************************************/
struct ConsolePlane {
  // If a menu plane is provided then it will register itself.
  ConsolePlane( maybe<MenuPlane&> menu_plane );
  ~ConsolePlane();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

} // namespace rn
