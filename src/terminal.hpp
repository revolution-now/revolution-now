/****************************************************************
**terminal.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-25.
*
* Description: Backend for lua terminal.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "expect.hpp"
#include "maybe.hpp"

// C++ standard library
#include <string>
#include <string_view>

namespace rn::term {

// This function is thread safe.
void log( std::string_view msg );

valid_or<std::string> run_cmd( std::string const& cmd );

// This function is thread safe.
void clear();

// idx zero is most recent. This function is thread safe.
maybe<std::string const&> line( int idx );

// idx zero is most recent.
maybe<std::string const&> history( int idx );

// Given a fragment of Lua this will return a vector of all pos-
// sible (immediate) completions. If it returns an empty vector
// then that means the fragment is invalid (i.e., it is not a
// prefix of any valid completion).
std::vector<std::string> autocomplete(
    std::string_view fragment );

// Will keep autocompleting so long as there is a single result,
// until the result converges and stops changing.
std::vector<std::string> autocomplete_iterative(
    std::string_view fragment );

} // namespace rn::term
