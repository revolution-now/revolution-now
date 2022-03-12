/****************************************************************
**renderer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-12.
*
* Description: Top-level generic (mostly game-independent) 2D
*              renderer.
*
*****************************************************************/
#pragma once

// render
#include "painter.hpp"
#include "sprite-sheet.hpp"
#include "typer.hpp"

// base
#include "base/macros.hpp"

// C++ standard library
#include <memory>
#include <string>
#include <unordered_map>

namespace rr {

/****************************************************************
** RendererConfig
*****************************************************************/
struct RendererConfig {
  gfx::size                         logical_screen_size = {};
  gfx::size                         max_atlas_size      = {};
  std::vector<SpriteSheetConfig>    sprite_sheets       = {};
  std::vector<AsciiFontSheetConfig> font_sheets         = {};
};

/****************************************************************
** Renderer
*****************************************************************/
// The video driver must have been fully initialized before using
// this.
struct Renderer {
  static Renderer create( RendererConfig const& config );

  ~Renderer() noexcept;

  // Must be called each time the logical screen size changes.
  void set_logical_screen_size( gfx::size new_size );

  void clear_screen( gfx::pixel color );

  // This must be called before any other rendering methods that
  // might generate vertices.
  void begin_pass();

  // This will end the rendering pass and upload the vertex data
  // to the GPU.  Returns the number of vertices uploaded.
  int end_pass();

  Painter painter();

  Typer typer( std::string_view font_name, gfx::point start,
               gfx::pixel color );

  Typer typer( std::string_view font_name, gfx::point start,
               gfx::pixel color, Painter& painter );

  // Given a globally unique name for a texture in the atlas,
  // this will return its id for use when rendering it.
  std::unordered_map<std::string_view, int> const& atlas_ids()
      const;

 private:
  NO_COPY_NO_MOVE( Renderer );

  struct Impl;

  Renderer( Impl* impl );

  // Can't use unique_ptr here because it needs visibility into
  // the destructor of Impl.
  Impl* impl_ = nullptr;
};

} // namespace rr
