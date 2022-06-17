/****************************************************************
**cheat.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-16.
*
* Description: Implements cheat mode.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

// Rds
#include "cheat.rds.hpp"

namespace rn {

struct Colony;
struct IGui;

/****************************************************************
** In Colony View
*****************************************************************/
wait<> cheat_colony_buildings( Colony& colony, IGui& gui );

} // namespace rn
