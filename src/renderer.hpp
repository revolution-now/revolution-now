/****************************************************************
**renderer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-19.
*
* Description: Holds and initializes the global renderer.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// render
#include "render/renderer.hpp"

namespace rn {

// Don't call this before the renderer is created/initialized.
rr::Renderer& global_renderer_use_only_when_needed();

} // namespace rn
