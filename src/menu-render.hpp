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

namespace rr {
struct Renderer;
}

namespace rn {

struct IMenuServer;

enum class e_menu_item;

/****************************************************************
** MenuAnimState
*****************************************************************/
struct MenuAnimState {
  maybe<std::string> highlighted;
  double alpha = 1.0;
};

/****************************************************************
** Rendered Layouts.
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
** Menu Rendering.
*****************************************************************/
void render_menu_body( rr::Renderer& renderer,
                       MenuAnimState const& state,
                       MenuRenderLayout const& layout,
                       IMenuServer const& menu_plane );

} // namespace rn
