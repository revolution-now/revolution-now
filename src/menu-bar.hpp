/****************************************************************
**menu-bar.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-22.
*
* Description: Runs the top menu bar.
*
*****************************************************************/
#pragma once

// rds
#include "menu-bar.rds.hpp"

// Revolution Now
#include "imenu-server.rds.hpp"
#include "wait.hpp"

namespace rn {

struct IMenuServer;
struct MenuBarAnimState;
struct MenuBarRenderedLayout;
struct MenuHeaderRenderLayout;

enum class e_menu;

/****************************************************************
** MenuBar
*****************************************************************/
struct MenuBar {
  MenuBar( IMenuServer& menu_server );
  ~MenuBar();

  wait<> run_thread(
      std::vector<e_menu> const& contents ATTR_LIFETIMEBOUND );

  [[nodiscard]] bool send_event( MenuBarEventRaw const& event );

  // NOTE: these references will go out of scope when the bar is
  // reloaded or brought down.
  MenuBarAnimState const& anim_state() const;
  MenuBarRenderedLayout const& render_layout() const;

 private:
  struct BarState;

  wait<> translate_input_thread( BarState& st );

  maybe<e_menu> header_from_point( gfx::point p ) const;

  maybe<MenuHeaderRenderLayout const&> layout_for_menu(
      e_menu menu ) const;

  void send_click( e_menu_item item ) const;

  bool handle_key_event( input::key_event_t const& key_event );

  bool handle_alt_key( input::e_key_change const change );

  bool handle_alt_shortcut(
      input::key_event_t const& key_event );

  BarState& state();
  BarState const& state() const;

  IMenuServer& menu_server_;
  std::unique_ptr<BarState> state_;
};

} // namespace rn
