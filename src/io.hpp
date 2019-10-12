/****************************************************************
**io.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-12.
*
* Description: Input/Output.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"

namespace rn {

expect<std::string> read_file_as_string( fs::path const& p );

/****************************************************************
** Testing
*****************************************************************/
void test_io();

} // namespace rn
