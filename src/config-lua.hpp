/****************************************************************
**config-lua.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-17.
*
* Description: Loads the config objects into a Lua state.
*
*****************************************************************/
#pragma once

namespace lua {
struct state;
}

namespace rn {

// Makes a deep conversion of all global config objects to their
// corresponding Lua representations (via a C++ -> CDR -> Lua)
// conversion as a simple way to expose the global config data to
// Lua. This is expensive... only use when needed, which, outside
// of unit tests, means calling it once when the Lua state is
// created.
//
// NOTE: this will NOT freeze the resulting tables, even though
// it is desired that they be frozen. For freezing them we should
// use the existing mechanism that we have that freezes all
// global tables (recursively) after full Lua initialization.
void inject_configs( lua::state& st );

} // namespace rn
