/****************************************************************
**menu-plane.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-14.
*
* Description: Implements the IMenuPlane menu server.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "imenu-plane.hpp"

// C++ standard library
#include <memory>

namespace rn {

/****************************************************************
** Menu2Plane
*****************************************************************/
struct Menu2Plane : IMenuPlane {
  Menu2Plane();

  wait<maybe<e_menu_item>> open_menu(
      MenuLayout const& layout,
      MenuPosition const& position ) override;

  ~Menu2Plane() override;

 public: // IMenPlane
  IPlane& impl() override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace rn
