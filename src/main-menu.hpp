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
struct Planes;

struct MainMenuPlane {
  MainMenuPlane( Planes& planes );
  ~MainMenuPlane() noexcept;

  wait<> item_selected( IGui& gui, e_main_menu_item item );

  wait<> run();

 private:
  Planes& planes_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace rn
