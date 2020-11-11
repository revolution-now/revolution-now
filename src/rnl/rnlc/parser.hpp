/****************************************************************
**parser.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: RNL Parser.
*
*****************************************************************/
#pragma once

// rnlc
#include "expr.hpp"

// c++ standard library
#include <optional>

namespace rnl {

std::optional<expr::Rnl> parse( std::string const& peg_grammar,
                                std::string const& rnl_text );

}
