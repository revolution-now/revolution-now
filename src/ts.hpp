/****************************************************************
**ts.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-25.
*
* Description: Non-serialized (transient) game state.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

namespace lua {
struct state;
}

namespace rn {

struct Planes;
struct IMapUpdater;
struct IGui;

/****************************************************************
** TS
*****************************************************************/
struct TS {
  IMapUpdater& map_updater;
  lua::state&  lua;
  IGui&        gui;
};

} // namespace rn
