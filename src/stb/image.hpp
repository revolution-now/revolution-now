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
#include "base/expect.hpp"
#include "base/fs.hpp"
#include "base/valid.hpp"

namespace stb {

base::expect<gfx::image> load_image( fs::path const& p );

base::valid_or<std::string> save_image(
    fs::path const& p, gfx::image const& image );

} // namespace stb
