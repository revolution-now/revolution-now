/****************************************************************
**ivideo.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-27.
*
* Description: Interface for dealing with the program window.
*
*****************************************************************/
#pragma once

// rds
#include "ivideo.rds.hpp"

// base
#include "base/expect.hpp"

namespace vid {

struct error {
  std::source_location loc = std::source_location::current();
  std::string msg;
};

template<typename T>
using or_err = base::expect<T, error>;

/****************************************************************
** IVideo
*****************************************************************/
struct IVideo {
  virtual ~IVideo() = default;

  virtual or_err<DisplayMode> display_mode() = 0;

  virtual or_err<WindowHandle> create_window(
      WindowOptions const& options ) = 0;
};

} // namespace vid
