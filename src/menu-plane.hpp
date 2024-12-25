/****************************************************************
**menu-plane.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-14.
*
* Description: Implements the IMenuServer menu server.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "imenu-server.hpp"

// C++ standard library
#include <memory>

namespace rn {

/****************************************************************
** MenuPlane
*****************************************************************/
struct MenuPlane : IMenuServer {
  MenuPlane();
  ~MenuPlane() override;

 public: // IMenuServer
  IPlane& impl() override;

  bool can_handle_menu_click( e_menu_item item ) const override;

  bool click_item( e_menu_item item ) override;

  wait<maybe<e_menu_item>> open_menu(
      e_menu menu, gfx::rect logical_screen_rect,
      MenuAllowedPositions const& positions
          ATTR_LIFETIMEBOUND ) override;

  void close_all_menus() override;

  void show_menu_bar( bool show ) override;

  void enable_cheat_menu( bool show ) override;

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
