/****************************************************************
**main-menu.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Main application menu.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

// Rds
#include "main-menu.rds.hpp"

// C++ standard library
#include <memory>

namespace rn {

struct IGui;
struct MenuPlane;
struct Plane;
struct Planes;
struct WindowPlane;

/****************************************************************
** MainMenuPlane
*****************************************************************/
struct MainMenuPlane {
  MainMenuPlane( Planes& planes, WindowPlane& window_plane,
                 IGui& gui );
  ~MainMenuPlane();

  wait<> run();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

} // namespace rn
