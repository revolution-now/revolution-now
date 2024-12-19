/****************************************************************
**extra.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-03.
*
* Description: Higher level rendering utilities.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/pixel.hpp"

namespace rr {

struct Painter;
struct Renderer;

void draw_empty_rect_no_corners( rr::Painter& painter,
                                 gfx::rect box,
                                 gfx::pixel color );

void render_shadow_hightlight_border( rr::Renderer& renderer,
                                      gfx::rect rect,
                                      gfx::pixel left_and_bottom,
                                      gfx::pixel top_and_right );

} // namespace rr
