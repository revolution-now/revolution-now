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
** MenuRenderState
*****************************************************************/
struct MenuRenderState {
  MenuLayout layout;
  MenuPosition position;
  maybe<e_menu_item> highlighted;
  double alpha = 1.0;
};

/****************************************************************
** MenuRenderer
*****************************************************************/
struct MenuRenderer {
  MenuRenderer( MenuRenderState const& state );

  gfx::rect bounding_rect() const;

  void render_body( rr::Renderer& renderer ) const;

 private:
  // These are static for safety because they are called during
  // the constructor.
  static gfx::size compute_bounding_size(
      MenuRenderState const& state );
  static gfx::rect compute_bounding_rect(
      MenuRenderState const& state );

  MenuRenderState const& state_;
  gfx::rect const bounds_;
};

} // namespace rn
