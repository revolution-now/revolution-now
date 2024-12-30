/****************************************************************
**difficulty-screen-2.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-11-03.
*
* Description: Screen where player chooses difficulty level.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "wait.hpp"

namespace rn {

struct IEngine;
struct Planes;

enum class e_difficulty;

wait<e_difficulty> choose_difficulty_screen_2( IEngine& engine,
                                               Planes& planes );

} // namespace rn
