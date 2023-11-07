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

enum class rcl_dialect {
  standard,
  json,
};

std::string save_rcl_to_string( ColonySAV const& in,
                                rcl_dialect      dialect );
std::string save_json_to_string( ColonySAV const& in );

base::valid_or<std::string> save_rcl_to_file(
    std::string const& path, ColonySAV const& in,
    rcl_dialect dialect );
base::valid_or<std::string> save_json_to_file(
    std::string const& path, ColonySAV const& in );

// Note there are no "load_json*" methods, because JSON is a
// subset of rcl.
base::valid_or<std::string> load_rcl_from_string(
    std::string const& filename_for_error_reporting,
    std::string const& in, ColonySAV& out );
base::valid_or<std::string> load_rcl_from_file(
    std::string const& path, ColonySAV& out );

} // namespace sav
