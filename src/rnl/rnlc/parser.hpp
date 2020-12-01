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

// base
#include "base/maybe.hpp"

namespace rnl {

base::maybe<expr::Rnl> parse( std::string_view   peg_filename,
                              std::string_view   src_filename,
                              std::string const& peg_grammar,
                              std::string const& rnl_text );

}
