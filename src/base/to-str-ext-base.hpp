/****************************************************************
**to-str-ext-base.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-18.
*
* Description: to_str implementations to base types.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "adl-tag.hpp"
#include "source-loc.hpp"

// C++ standard library
#include <string>

namespace base {

void to_str( base::SourceLoc const& o, std::string& out, ADL_t );

} // namespace base