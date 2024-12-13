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

// gfx
#include "gfx/matrix.hpp"

namespace rr {
struct Renderer;
}

namespace rn {

struct SS;

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

 public: // Implement IMapUpdater.
  BuffersUpdated modify_map_square( Coord,
                                    SquareUpdateFunc ) override;

  std::vector<BuffersUpdated> make_squares_visible(
      e_nation nation,
      std::vector<Coord> const& tiles ) override;

  std::vector<BuffersUpdated> make_squares_fogged(
      e_nation nation,
      std::vector<Coord> const& tiles ) override;

  std::vector<BuffersUpdated> force_redraw_tiles(
      std::vector<Coord> const& tiles ) override;

  void modify_entire_map_no_redraw(
      MapUpdateFunc mutator ) override;

  void redraw() override;

  void unrender() override;

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

 public: // Implement IMapUpdater.
  BuffersUpdated modify_map_square(
      Coord tile, SquareUpdateFunc mutator ) override;

  std::vector<BuffersUpdated> make_squares_visible(
      e_nation nation,
      std::vector<Coord> const& tiles ) override;

  std::vector<BuffersUpdated> make_squares_fogged(
      e_nation nation,
      std::vector<Coord> const& tiles ) override;

  std::vector<BuffersUpdated> force_redraw_tiles(
      std::vector<Coord> const& tiles ) override;

  void modify_entire_map_no_redraw(
      MapUpdateFunc mutator ) override;

  void redraw() override;

  void unrender() override;

 private:
  void redraw_buffers_for_tiles_where_needed(
      std::vector<BuffersUpdated> const& buffers_updated );

  void redraw_landscape_buffer();
  void redraw_obfuscation_buffer();

  struct BufferTracking {
    BufferTracking( Delta size ) : tile_bounds( size ) {}

    int tiles_redrawn = 0;
    gfx::Matrix<rr::VertexRange> tile_bounds;
  };

  void redraw_square_single_buffer(
      Coord tile, BufferTracking& buffer_tracking,
      rr::e_render_buffer annex_buffer,
      base::function_ref<void()> render_square,
      base::function_ref<void()> redraw_buffer );

  rr::Renderer& renderer_;
  BufferTracking landscape_tracking_;
  BufferTracking obfuscation_tracking_;
};

} // namespace rn
