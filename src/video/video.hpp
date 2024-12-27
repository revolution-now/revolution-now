/****************************************************************
**video.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-27.
*
* Description: Implementation of IVideo using an SDL backend.
*
*****************************************************************/
#pragma once

// video
#include "ivideo.hpp"

namespace vid {

/****************************************************************
** SDLVideo
*****************************************************************/
struct SDLVideo : IVideo {
  or_err<DisplayMode> display_mode() override;

  or_err<WindowHandle> create_window(
      WindowOptions const& options ) override;
};

} // namespace vid
