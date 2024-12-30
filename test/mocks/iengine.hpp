/****************************************************************
**iengine.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-30.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/iengine.hpp"

// mock
#include "src/mock/mock.hpp"

namespace rn {

/****************************************************************
** MockIEngine
*****************************************************************/
struct MockIEngine : IEngine {
  MOCK_METHOD( vid::IVideo&, video, (), () );
  MOCK_METHOD( sfx::ISfx&, sfx, (), () );
  MOCK_METHOD( vid::WindowHandle const&, window, (), () );
  MOCK_METHOD( rr::Renderer&, renderer_use_only_when_needed, (),
               () );
  MOCK_METHOD( gfx::Resolutions&, resolutions, (), () );
};

static_assert( !std::is_abstract_v<MockIEngine> );

} // namespace rn
