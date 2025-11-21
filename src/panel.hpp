/****************************************************************
**panel.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-11-15.
*
* Description: Implementation for the panel.
*
*****************************************************************/
#pragma once

// rds
#include "panel.rds.hpp"

// ss
#include "ss/unit-id.hpp"

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/maybe.hpp"

namespace rr {
struct Renderer;
struct ITextometer;
}

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IVisibility;
struct SSConst;

/****************************************************************
** Public API.
*****************************************************************/
PanelEntities entities_shown_on_panel( SSConst const& ss,
                                       IVisibility const& viz );

PanelLayout panel_layout( SSConst const& ss,
                          IVisibility const& viz,
                          PanelEntities const& entities );

PanelRenderPlan panel_render_plan(
    SSConst const& ss, IVisibility const& viz,
    rr::ITextometer const& textometer, PanelLayout const& layout,
    int panel_width );

void render_panel_layout( rr::Renderer& renderer,
                          SSConst const& ss, gfx::point where,
                          IVisibility const& viz,
                          PanelRenderPlan const& plan );

} // namespace rn
