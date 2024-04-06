/****************************************************************
**fs.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-15.
*
* Description: std::filesystem wrappers.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "valid.hpp"

// C++ standard library
#include <filesystem>

namespace fs = ::std::filesystem;

namespace base {

// The std version of this uses a combination of return values
// and exceptions for error handling which we don't want. Using
// this wrapper will be friendlier.
valid_or<std::string> copy_file_overwriting_destination(
    fs::path const& from, fs::path const& to );

}