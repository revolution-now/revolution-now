/****************************************************************
**binary.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-01.
*
* Description: API for loading and saving games in the classic
*              format of the OG.
*
*****************************************************************/
#pragma once

// base
#include "base/valid.hpp"

// C++ standard library.
#include <string>

namespace sav {

struct ColonySAV;
struct BinaryFile;

base::valid_or<std::string> load_binary( std::string const& path,
                                         ColonySAV& out );

base::valid_or<std::string> save_binary( std::string const& path,
                                         ColonySAV const&   in );

} // namespace sav
