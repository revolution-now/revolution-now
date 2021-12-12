/****************************************************************
**image.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-04.
*
* Description: C++ wrapper around stb_image.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/image.hpp"

// base
#include "base/fs.hpp"

namespace stb {

gfx::image load_image( fs::path const& p );

} // namespace stb
