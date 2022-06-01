/****************************************************************
**colony-mfg.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-18.
*
* Description: All things related to the jobs colonists can have
*              by working in the buildings in the colony.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "lua-enum.hpp"

// Rds
#include "colony-mfg.rds.hpp"

namespace rn {

void linker_dont_discard_module_colony_mfg();

LUA_ENUM_DECL( colony_building );

} // namespace rn
