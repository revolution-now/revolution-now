/****************************************************************
**rcl.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-07.
*
* Description: API for loading and saving games in the classic
*              format of the OG but represented in rcl/json.
*
*****************************************************************/
#pragma once

// base
#include "base/valid.hpp"

// C++ standard library.
#include <string>

namespace sav {

struct ColonySAV;

enum class e_rcl_dialect {
  standard,
  json,
};

std::string save_rcl_to_string( ColonySAV const& in,
                                e_rcl_dialect    dialect );

base::valid_or<std::string> save_rcl_to_file(
    std::string const& path, ColonySAV const& in,
    e_rcl_dialect dialect );

// Note we don't need to specify the dialect for the load methods
// because JSON is a subset of rcl.
base::valid_or<std::string> load_rcl_from_string(
    std::string const& filename_for_error_reporting,
    std::string const& in, ColonySAV& out );

base::valid_or<std::string> load_rcl_from_file(
    std::string const& path, ColonySAV& out );

} // namespace sav
