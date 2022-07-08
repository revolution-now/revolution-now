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

namespace lua {
struct state;
}

namespace rn {

struct Plane;
struct Terminal;

/****************************************************************
** ConsolePlane
*****************************************************************/
struct ConsolePlane {
  ConsolePlane( Terminal& terminal );
  ~ConsolePlane();

  Terminal& terminal();

  lua::state& lua_state();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

} // namespace rn
