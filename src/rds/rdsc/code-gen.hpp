/****************************************************************
**code-gen.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: Code generator for the RDS language.
*
*****************************************************************/
#pragma once

// rdsc
#include "expr.hpp"

// base
#include "base/maybe.hpp"

// c++ standard library
#include <string>

namespace rds {

base::maybe<std::string> generate_code( expr::Rds const& rds );

} // namespace rds
