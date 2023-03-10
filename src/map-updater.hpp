/****************************************************************
**map-updater.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-04.
*
* Description: Concrete implementations of IMapUpdater.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "imap-updater.hpp"

// render
#include "render/renderer.rds.hpp"

namespace rr {
struct Renderer;
}

namespace rn {

struct SS;
struct TerrainRenderOptions;
struct Visibility;

/****************************************************************
** NonRenderingMapUpdater
*****************************************************************/
// This one does the updating of the map but not the rendering.
// It is useful in unit tests where the map updater will be
// called but we don't want to mock it.
struct NonRenderingMapUpdater : IMapUpdater {
  NonRenderingMapUpdater( SS& ss ) : ss_( ss ) {}

  // Implement IMapUpdater.
  bool modify_map_square( Coord, SquareUpdateFunc ) override;

  // Implement IMapUpdater.
  bool make_square_visible( Coord    tile,
                            e_nation nation ) override;

  // Implement IMapUpdater.
  bool make_square_fogged( Coord    tile,
                           e_nation nation ) override;

  // Implement IMapUpdater.
  void modify_entire_map( MapUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  void redraw() override;

 protected:
  SS& ss_;
};

/****************************************************************
** RenderingMapUpdater
*****************************************************************/
// The real map updater used by the game. This one delegates to
// the non-rendering version to make changes to the maps, then
// proceeds (if necessary) to do rendering.
struct RenderingMapUpdater : NonRenderingMapUpdater {
  using Base = NonRenderingMapUpdater;

  RenderingMapUpdater( SS& ss, rr::Renderer& renderer );

  // Implement IMapUpdater.
  bool modify_map_square( Coord            tile,
                          SquareUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  bool make_square_visible( Coord    tile,
                            e_nation nation ) override;

  // Implement IMapUpdater.
  bool make_square_fogged( Coord    tile,
                           e_nation nation ) override;

  // Implement IMapUpdater.
  void modify_entire_map( MapUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  void redraw() override;

 private:
  void redraw_square(
      Visibility const&           viz,
      TerrainRenderOptions const& terrain_options, Coord tile );

  rr::Renderer&           renderer_;
  int                     tiles_redrawn_;
  Matrix<rr::VertexRange> tile_bounds_;
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
  bool modify_map_square( Coord, SquareUpdateFunc ) override;

  // Implement IMapUpdater.
  bool make_square_visible( Coord    tile,
                            e_nation nation ) override;

  // Implement IMapUpdater.
  bool make_square_fogged( Coord    tile,
                           e_nation nation ) override;

  // Implement IMapUpdater.
  void modify_entire_map( MapUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  void redraw() override;
};

} // namespace rn
