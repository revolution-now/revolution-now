/****************************************************************
**autosave.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-05.
*
* Description: Handles logic related to autosaving.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "expect.hpp"

// base
#include "base/fs.hpp"

// C++ standard library
#include <vector>

namespace rn {

struct Autosave;
struct IGameSaver;
struct SSConst;

// Returns the list of autosave slots that need to be saved to
// this turn, where slot 0 is the first autosave slot.
[[nodiscard]] std::vector<int> should_autosave(
    SSConst const& autosave );

// This will do the save.
[[nodiscard]] expect<std::vector<fs::path>> autosave(
    SSConst const& ss, IGameSaver const& game_saver,
    Autosave& autosave, std::vector<int> const& autosave_slots );

} // namespace rn
