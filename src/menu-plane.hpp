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
  ~Menu2Plane() override;

 public: // IMenuPlane
  IPlane& impl() override;

  bool can_handle_menu_click( e_menu_item item ) const override;

  bool click_item( e_menu_item item ) override;

  wait<maybe<e_menu_item>> open_menu(
      MenuContents const& contents ATTR_LIFETIMEBOUND,
      MenuAllowedPositions const& positions
          ATTR_LIFETIMEBOUND ) override;

  Deregistrar register_handler( e_menu_item item,
                                IPlane& plane ) override;

 private:
  void unregister_handler( e_menu_item item,
                           IPlane& plane ) override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace rn
