/****************************************************************
**auto-complete.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-31.
*
* Description: Auto-complete for Lua in the game terminal.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <string>
#include <string_view>
#include <vector>

namespace lua {
struct state;
}

namespace rn {

// Given a fragment of Lua this will return a vector of all pos-
// sible (immediate) completions. If it returns an empty vector
// then that means the fragment is invalid (i.e., it is not a
// prefix of any valid completion).
std::vector<std::string> autocomplete(
    lua::state& st, std::string_view fragment );

// Will keep auto-completing so long as there is a single result,
// until the result converges and stops changing.
std::vector<std::string> autocomplete_iterative(
    lua::state& st, std::string_view fragment );

} // namespace rn
