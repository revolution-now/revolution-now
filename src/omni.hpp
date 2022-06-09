/****************************************************************
**omni.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-08.
*
* Description: A plane that is always on the top of the stack.
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

/****************************************************************
** OmniPlane
*****************************************************************/
struct OmniPlane {
  OmniPlane( Planes& planes, e_plane_stack where );
  ~OmniPlane() noexcept;

 private:
  Planes&             planes_;
  e_plane_stack const where_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace rn
