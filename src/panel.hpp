/****************************************************************
**panel.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-11-01.
*
* Description: The side panel on land view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

// Rds
#include "plane-stack.rds.hpp"

namespace rn {

struct MenuPlane;
struct Planes;

/****************************************************************
** PanelPlane
*****************************************************************/
struct PanelPlane {
  PanelPlane( Planes& planes, e_plane_stack where,
              MenuPlane& menu_plane );
  ~PanelPlane() noexcept;

  wait<> wait_for_eot_button_click();

 private:
  Planes&             planes_;
  e_plane_stack const where_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace rn
