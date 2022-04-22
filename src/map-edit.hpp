/****************************************************************
**map-edit.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-03.
*
* Description: Map Editor.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "map-updater.hpp"
#include "wait.hpp"

// Rds
#include "map-edit.rds.hpp"

// render
#include "render/renderer.hpp"

namespace rn {

wait<> map_editor( IMapUpdater& map_updater );

struct Plane;
Plane* map_editor_plane();

} // namespace rn
