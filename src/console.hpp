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

// Rds
#include "plane-stack.rds.hpp"

// C++ standard library
#include <memory>

namespace rn {

struct Planes;
struct MenuPlane;

/****************************************************************
** ConsolePlane
*****************************************************************/
struct ConsolePlane {
  ConsolePlane( Planes& planes, e_plane_stack where,
                MenuPlane& menu_plane );
  ~ConsolePlane() noexcept;

 private:
  Planes&             planes_;
  e_plane_stack const where_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace rn
