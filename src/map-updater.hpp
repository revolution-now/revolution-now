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
  NonRenderingMapUpdater( SS& ss );

  NonRenderingMapUpdater(
      SS& ss, MapUpdaterOptions const& initial_options );

  // Implement IMapUpdater.
  BuffersUpdated modify_map_square( Coord,
                                    SquareUpdateFunc ) override;

  // Implement IMapUpdater.
  BuffersUpdated make_square_visible( Coord    tile,
                                      e_nation nation ) override;

  // Implement IMapUpdater.
  BuffersUpdated make_square_fogged( Coord    tile,
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

  RenderingMapUpdater(
      SS& ss, rr::Renderer& renderer,
      MapUpdaterOptions const& initial_options );

  // Implement IMapUpdater.
  BuffersUpdated modify_map_square(
      Coord tile, SquareUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  BuffersUpdated make_square_visible( Coord    tile,
                                      e_nation nation ) override;

  // Implement IMapUpdater.
  BuffersUpdated make_square_fogged( Coord    tile,
                                     e_nation nation ) override;

  // Implement IMapUpdater.
  void modify_entire_map( MapUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  void redraw() override;

 private:
  BuffersUpdated redraw_buffers_for_tile_where_needed(
      Coord tile, BuffersUpdated const& buffers_updated );

  void redraw_landscape_buffer();
  void redraw_obfuscation_buffer();

  struct BufferTracking {
    BufferTracking( Delta size ) : tile_bounds( size ) {}

    int                     tiles_redrawn = 0;
    Matrix<rr::VertexRange> tile_bounds;
  };

  void redraw_square_single_buffer(
      Coord tile, BufferTracking& buffer_tracking,
      rr::e_render_buffer        annex_buffer,
      base::function_ref<void()> render_square,
      base::function_ref<void()> redraw_buffer );

  rr::Renderer&  renderer_;
  BufferTracking landscape_tracking_;
  BufferTracking obfuscation_tracking_;
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
  BuffersUpdated modify_map_square( Coord,
                                    SquareUpdateFunc ) override;

  // Implement IMapUpdater.
  BuffersUpdated make_square_visible( Coord    tile,
                                      e_nation nation ) override;

  // Implement IMapUpdater.
  BuffersUpdated make_square_fogged( Coord    tile,
                                     e_nation nation ) override;

  // Implement IMapUpdater.
  void modify_entire_map( MapUpdateFunc mutator ) override;

  // Implement IMapUpdater.
  void redraw() override;
};

} // namespace rn
