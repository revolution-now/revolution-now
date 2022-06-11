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

struct MenuPlane;
struct Plane;
struct Planes;
struct WindowPlane;

/****************************************************************
** MainMenuPlane
*****************************************************************/
struct MainMenuPlane {
  MainMenuPlane( Planes& planes, WindowPlane& window_plane );
  ~MainMenuPlane();

  wait<> run();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

/****************************************************************
** API
*****************************************************************/
wait<> run_main_menu( Planes& planes );

} // namespace rn
