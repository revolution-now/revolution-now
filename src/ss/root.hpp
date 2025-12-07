/****************************************************************
**root.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-14.
*
* Description: Top-level struct representing the data that is
*              saved when a game is saved.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "root.rds.hpp"

// luapp
#include "luapp/ext-usertype.hpp"

namespace rn {

void define_usertype_for( lua::state& st, lua::tag<RootState> );

// Run a full recursive validation over all fields.
base::valid_or<std::string> validate_recursive(
    RootState const& root );

// Faster because it omits terrain.
base::valid_or<std::string> validate_recursive_non_terrain(
    RootState const& root );

} // namespace rn
