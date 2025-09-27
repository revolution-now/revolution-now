/****************************************************************
**igoto-viewer.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-27.
*
* Description: Interface for goto path-finding algorithms to
*              query the map they are searching.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"

namespace rn {

/****************************************************************
** IGotoMapViewer
*****************************************************************/
// The A-Star (or similar) algorithms that compute unit travel
// paths will use this interface to query the map and to deter-
// mine if a given tile is traversible. It is good to put this
// behind an interface because there are various factors that go
// into answering that question.
struct IGotoMapViewer {
  virtual ~IGotoMapViewer() = default;

  [[nodiscard]] virtual bool can_enter_tile(
      gfx::point tile ) const = 0;
};

} // namespace rn
