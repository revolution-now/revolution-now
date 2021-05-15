/****************************************************************
**build-properties.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-23.
*
* Description: Utilities for getting info about the build.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "fs.hpp"

namespace base {

// Full, absolute, canonical path to the root of the project
// source tree on the build machine during the build. This is the
// "revolution-now-game" folder, under which there is "src".
fs::path const& source_tree_root();

// Full, absolute, canonical path to the root of the binary
// output folder on the build machine during the build.
fs::path const& build_output_root();

} // namespace base