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
#include "imenu-plane.rds.hpp"
#include "maybe.hpp"

namespace rr {
struct Renderer;
}

namespace rn {

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
  std::string text          = {};
  gfx::rect bounds_relative = {};
  gfx::rect bounds_absolute = {};
  bool has_arrow            = {};
};

struct MenuRenderLayout {
  gfx::rect bounds;
  std::vector<MenuItemRenderLayout> items;
  std::vector<gfx::rect /*relative*/> bars;
};

MenuRenderLayout build_menu_rendered_layout(
    MenuContents const& contents, MenuPosition const& position );

/****************************************************************
** MenuRenderer
*****************************************************************/
void render_menu_body( rr::Renderer& renderer,
                       MenuAnimState const& state,
                       MenuRenderLayout const& layout );

} // namespace rn
