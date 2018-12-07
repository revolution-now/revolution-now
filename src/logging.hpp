/****************************************************************
**logging.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-07.
*
* Description: Interface to logging
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// clang-format will reorder these headers which then generates
// an error because they need to be included in a certian order.
// clang-format off
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
// clang-format on

// c++ standard library
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace {

// This will create one logger for each translation unit.  The
// name of the logger will be the stem of the filename of the
// cpp file.  Using __FILE__ would yield the name of this header
// file which would not be useful.
auto console = spdlog::stdout_color_mt(
    fs::path( __BASE_FILE__ ).filename().stem() );

} // namespace

namespace rn {} // namespace rn
