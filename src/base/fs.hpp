/****************************************************************
**fs.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-15.
*
* Description: Include this to use std::filesystem.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// C++ standard library
#include <filesystem>
#include <string>

namespace fs = ::std::filesystem;

namespace std::filesystem {

inline void to_str( path const& p, std::string& out ) {
  out += p.string();
}

} // namespace std::filesystem