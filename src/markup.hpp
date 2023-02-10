/****************************************************************
**markup.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-09.
*
* Description: Parser for text markup mini-language.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "markup.rds.hpp"

// base
#include "base/expect.hpp"

// C++ standard library
#include <string_view>
#include <vector>

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
base::expect<MarkedUpText> parse_markup( std::string_view text );

std::string remove_markup( std::string_view text );

} // namespace rn
