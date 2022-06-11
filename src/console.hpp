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

struct Plane;

/****************************************************************
** ConsolePlane
*****************************************************************/
struct ConsolePlane {
  ConsolePlane();
  ~ConsolePlane();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

} // namespace rn
