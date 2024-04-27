/****************************************************************
**difficulty-screen.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-21.
*
* Description: Screen where player chooses difficulty level.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "wait.hpp"

// base
#include "base/vocab.hpp"

namespace rn {

struct Planes;

enum class e_difficulty;

wait<base::NoDiscard<e_difficulty>> choose_difficulty_screen(
    Planes& planes );

} // namespace rn
