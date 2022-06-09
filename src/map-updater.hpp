/****************************************************************
**map-updater.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-21.
*
* Description: Handlers when a map square needs to be modified.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "map-square.hpp"
#include "matrix.hpp"

// render
#include "render/renderer.hpp"

// base
#include "base/function-ref.hpp"

namespace rn {

struct TerrainState;

/****************************************************************
** IMapUpdater
*****************************************************************/
// This is an abstract class so that it can be injected and
// mocked, which is useful because there are places in the code
// that we want to test but that happen to also need to update
// map squares, and we want to be able to test those without
// having to worry about a dependency on the renderer.
struct IMapUpdater {
  using SquareUpdateFunc =
      base::function_ref<void( MapSquare& )>;
  using MapUpdateFunc =
      base::function_ref<void( Matrix<MapSquare>& )>;

  virtual ~IMapUpdater() = default;

  // This function should be used whenever a map square
  // (specifically, a MapSquare object) must be updated as it
  // will handler re-rendering the surrounding squares.
  virtual void modify_map_square( Coord            tile,
                                  SquareUpdateFunc mutator ) = 0;

  // This function should be used when generating the map.
  virtual void modify_entire_map( MapUpdateFunc mutator ) = 0;

  // Will redraw the entire map.
  virtual void just_redraw_map() = 0;
};

/****************************************************************
** MapUpdater
*****************************************************************/
struct MapUpdater : IMapUpdater {
  MapUpdater( TerrainState& terrain_state,
              rr::Renderer& renderer )
    : terrain_state_( terrain_state ),
      renderer_( renderer ),
      tiles_updated_( 0 ) {}

  // Implement IMapUpdater.
  void modify_map_square( Coord            tile,
                          SquareUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  void modify_entire_map( MapUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  void just_redraw_map() override;

 private:
  TerrainState& terrain_state_;
  rr::Renderer& renderer_;
  int           tiles_updated_;
};

/****************************************************************
** NonRenderingMapUpdater
*****************************************************************/
struct NonRenderingMapUpdater : IMapUpdater {
  NonRenderingMapUpdater( TerrainState& terrain_state )
    : terrain_state_( terrain_state ) {}

  // Implement IMapUpdater.
  void modify_map_square( Coord, SquareUpdateFunc ) override;

  // Implement IMapUpdater.
  void modify_entire_map( MapUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  void just_redraw_map() override;

 private:
  TerrainState& terrain_state_;
};

/****************************************************************
** TrappingMapUpdater
*****************************************************************/
// This one literally does nothing except for check-fail if any
// of its methods are called that attempt to modify the map. It's
// for when you know that the map updater will not be called, but
// need one to pass in anyway.
struct TrappingMapUpdater : IMapUpdater {
  TrappingMapUpdater() = default;

  // Implement IMapUpdater.
  void modify_map_square( Coord, SquareUpdateFunc ) override;

  // Implement IMapUpdater.
  void modify_entire_map( MapUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  void just_redraw_map() override;
};

} // namespace rn
