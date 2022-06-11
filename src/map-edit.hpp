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
#include "wait.hpp"

namespace rn {

struct IMapUpdater;
struct LandViewState;
struct MenuPlane;
struct Plane;
struct TerrainState;

/****************************************************************
** MapEditPlane
*****************************************************************/
struct MapEditPlane {
  MapEditPlane( IMapUpdater&        map_updater,
                LandViewState&      land_view_state,
                TerrainState const& terrain_state,
                MenuPlane&          menu_plane );
  ~MapEditPlane();

  wait<> run_map_editor();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

/****************************************************************
** API
*****************************************************************/
wait<> run_map_editor( IMapUpdater&        map_updater,
                       LandViewState&      land_view_state,
                       TerrainState const& terrain_state,
                       bool                standalone_mode );

} // namespace rn
