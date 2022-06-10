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
#include "plane-stack.hpp"
#include "wait.hpp"

namespace rn {

struct IMapUpdater;
struct LandViewState;
struct Planes;
struct TerrainState;

/****************************************************************
** MapEditPlane
*****************************************************************/
struct MapEditPlane {
  MapEditPlane( Planes& planes, e_plane_stack where,
                IMapUpdater&        map_updater,
                LandViewState&      land_view_state,
                TerrainState const& terrain_state );

  ~MapEditPlane() noexcept;

  wait<> map_editor();

  wait<> map_editor_standalone();

 private:
  Planes&             planes_;
  e_plane_stack const where_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace rn
