/****************************************************************
**config-files.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-11-29.
*
* Description: Handles config file data.
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace rn {

// This tells us if all of the configs have been fully loaded.
// This is useful for preventing bugs that would result from
// using the config data (global variables) before they are
// loaded from the contents of config files.
bool configs_loaded();

} // namespace rn
