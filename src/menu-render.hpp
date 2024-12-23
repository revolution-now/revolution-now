/****************************************************************
**menu-render.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-15.
*
* Description: Renders menus.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "imenu-server.rds.hpp"
#include "maybe.hpp"

// base
#include "base/function-ref.hpp"

namespace rr {
struct Renderer;
}

namespace rn {

enum class e_menu_item;
enum class e_menu;

/****************************************************************
** MenuAnimState
*****************************************************************/
struct MenuAnimState {
  maybe<std::string> highlighted;
  double alpha = 1.0;
};

/****************************************************************
** MenuBarAnimState
*****************************************************************/
enum class e_menu_highlight_state {
  highlighted,
  open,
};

struct MenuBarAnimState {
  maybe<e_menu> focused;
  // Only relevant if `focused` is populated.
  e_menu_highlight_state highlight_state = {};

  maybe<e_menu> highlighted_menu() const;
  maybe<e_menu> opened_menu() const;

  void set_highlighted( e_menu menu );
  void set_opened( e_menu menu );

  void clear_focus();
};

/****************************************************************
** Menu Body Rendered Layouts.
*****************************************************************/
// These should only hold things that don't change after cre-
// ation. Any rendering state that needs to change should be in
// MenuAnimState.
struct MenuItemRenderLayout {
  // This will have a value if this is a leaf item.
  maybe<e_menu_item> item     = {};
  std::string text            = {};
  gfx::rect bounds_relative   = {};
  gfx::rect bounds_absolute   = {};
  gfx::point text_nw_relative = {}; // relative to row origin.
  bool has_arrow              = {};
};

struct MenuRenderLayout {
  MenuPosition position;
  gfx::rect bounds;
  std::vector<MenuItemRenderLayout> items;
  std::vector<gfx::rect /*relative*/> bars;
};

// There must be at least one allowed position supplied.
MenuRenderLayout build_menu_rendered_layout(
    MenuContents const& contents,
    MenuAllowedPositions const& positions );

/****************************************************************
** Menu Bar Rendered Layouts.
*****************************************************************/
// These should only hold things that don't change after cre-
// ation. Any rendering state that needs to change should be in
// MenuAnimState.
struct MenuHeaderRenderLayout {
  e_menu menu                 = {};
  std::string text            = {};
  gfx::point text_nw_absolute = {};
  gfx::rect bounds_absolute   = {};
};

struct MenuBarRenderedLayout {
  gfx::rect bounds;
  std::vector<MenuHeaderRenderLayout> headers;
};

MenuBarRenderedLayout build_menu_bar_rendered_layout(
    MenuBarContents const& contents );

/****************************************************************
** Menu Rendering.
*****************************************************************/
using ItemEnabledFn = bool( MenuItemRenderLayout const& ) const;

void render_menu_body(
    rr::Renderer& renderer, MenuAnimState const& state,
    MenuRenderLayout const& layout,
    base::function_ref<ItemEnabledFn> const enabled_fn );

void render_menu_bar( rr::Renderer& renderer,
                      MenuBarAnimState const& anim_state,
                      MenuBarRenderedLayout const& layout );

} // namespace rn
