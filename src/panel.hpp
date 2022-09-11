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

namespace rn {

struct Plane;
struct Planes;
struct SS;
struct TS;

/****************************************************************
** PanelPlane
*****************************************************************/
struct PanelPlane {
  PanelPlane( Planes& planes, SS& ss, TS& ts );
  ~PanelPlane();

  wait<> wait_for_eot_button_click();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

} // namespace rn
