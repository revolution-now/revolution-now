/****************************************************************
**turn-plane.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-11.
*
* Description: A plane for the turn handler.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

// config
#include "config/menu-items.rds.hpp"

// C++ standard library
#include <memory>

namespace rn {

struct MenuPlane;
struct Plane;

/****************************************************************
** TurnPlane
*****************************************************************/
// This is mostly just to handle menu items.
struct TurnPlane {
  TurnPlane( MenuPlane& menu_plane );
  ~TurnPlane();

  wait<e_menu_item> next_menu_action();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

} // namespace rn
