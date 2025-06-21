/****************************************************************
**noise.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-21.
*
* Description: Creates the noise texture.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/image.hpp"

namespace rr {

gfx::image create_noise_image( gfx::size sz );

} // namespace rr
