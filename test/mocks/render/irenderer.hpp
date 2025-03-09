/****************************************************************
**irenderer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2024-10-04.
*
* Description: Mock implementation of IRenderer.
*
*****************************************************************/
#pragma once

// render
#include "src/render/irenderer.hpp"

// mock
#include "src/mock/mock.hpp"

namespace rr {

struct MockRenderer : IRenderer {
  MOCK_METHOD( void, set_color_cycle_stage, (int), () );
  MOCK_METHOD( void, set_color_cycle_plans,
               (std::vector<gfx::pixel> const&), () );
  MOCK_METHOD( void, set_color_cycle_keys,
               (std::span<gfx::pixel const>), () );
};

static_assert( !std::is_abstract_v<MockRenderer> );

} // namespace rr
