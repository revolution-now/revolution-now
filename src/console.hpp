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

struct IEngine;
struct IPlane;
struct Terminal;

/****************************************************************
** ConsolePlane
*****************************************************************/
struct ConsolePlane {
  ConsolePlane( IEngine& engine, Terminal& terminal,
                lua::state& st );
  ~ConsolePlane();

  Terminal& terminal() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  IPlane& impl();
};

} // namespace rn
