/****************************************************************
**colony-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-05.
*
* Description: The view that appears when clicking on a colony.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "colony-id.hpp"
#include "wait.hpp"

// Rds
#include "plane-stack.rds.hpp"

namespace rn {

struct Colony;
struct IGui;
struct Planes;

/****************************************************************
** ColonyPlane
*****************************************************************/
struct ColonyPlane {
  ColonyPlane( Planes& planes, e_plane_stack where,
               Colony& colony, IGui& gui );
  ~ColonyPlane() noexcept;

  wait<> show_colony_view() const;

 private:
  Planes&             planes_;
  e_plane_stack const where_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace rn
