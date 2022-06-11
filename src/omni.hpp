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

// C++ standard library
#include <memory>

namespace rn {

struct Plane;

/****************************************************************
** OmniPlane
*****************************************************************/
struct OmniPlane {
  OmniPlane();
  ~OmniPlane();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

} // namespace rn
