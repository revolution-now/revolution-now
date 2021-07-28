/****************************************************************
**rds-parser.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-21.
*
* Description: RDS Parser.
*
*****************************************************************/
#pragma once

// rnlc
#include "expr.hpp"

// base
#include "base/maybe.hpp"

namespace rnl {

base::maybe<expr::Rnl> parse_rds(
    std::string_view preamble_filename,
    std::string_view rds_filename );

}
