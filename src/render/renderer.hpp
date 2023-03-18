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

// Rds
#include "renderer.rds.hpp"

// render
#include "painter.hpp"
#include "sprite-sheet.hpp"
#include "typer.hpp"

// base
#include "base/function-ref.hpp"
#include "base/macros.hpp"

// C++ standard library
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace rr {

/****************************************************************
** Macros
*****************************************************************/
// This makes it easier to do a scoped change of the renderer
// mods.  Use it like so:
//
// {
//   SCOPED_RENDERER_MOD_SET( painter_mods.repos.scale, 2.0 );
//   SCOPED_RENDERER_MOD_ADD( painter_mods.repos.translation, 5
//   ); SCOPED_RENDERER_MOD_MUL( painter_mods.repos.scale, 1.5 );
//   ...
// }
//
// Note that one should prefer using the cumulative variants if
// possible so that nested scopes can compound their changes in-
// stead of overwriting them.
//
#define SCOPED_RENDERER_MOD_SET( leaf_path, ... )           \
  auto STRING_JOIN( __scoped_renderer_popper_, __LINE__ ) = \
      renderer.push_mods( [&]( rr::RendererMods& mods ) {   \
        mods.leaf_path = __VA_ARGS__;                       \
      } );

#define SCOPED_RENDERER_MOD_ADD( leaf_path, ... )           \
  auto STRING_JOIN( __scoped_renderer_popper_, __LINE__ ) = \
      renderer.push_mods( [&]( rr::RendererMods& mods ) {   \
        if( !mods.leaf_path.has_value() )                   \
          mods.leaf_path.emplace();                         \
        *mods.leaf_path += __VA_ARGS__;                     \
      } );

// This one only really works for numbers, since we have to ini-
// tialize it to something that is the identity, and it's hard to
// do that for types in general.
#define SCOPED_RENDERER_MOD_MUL( leaf_path, ... )             \
  static_assert(                                              \
      std::is_integral_v<std::remove_cvref_t<                 \
          decltype( *std::declval<::rr::RendererMods>()       \
                         .leaf_path )>> ||                    \
      std::is_floating_point_v<std::remove_cvref_t<           \
          decltype( *std::declval<::rr::RendererMods>()       \
                         .leaf_path )>> );                    \
  auto STRING_JOIN( __scoped_renderer_popper_, __LINE__ ) =   \
      renderer.push_mods( [&]( rr::RendererMods& mods ) {     \
        if( !mods.leaf_path.has_value() ) mods.leaf_path = 1; \
        *mods.leaf_path *= __VA_ARGS__;                       \
      } );

// This one is only for bools.
#define SCOPED_RENDERER_MOD_OR( leaf_path, ... )            \
  static_assert(                                            \
      std::is_same_v<                                       \
          bool,                                             \
          std::remove_cvref_t<                              \
              decltype( *std::declval<::rr::RendererMods>() \
                             .leaf_path )>> );              \
  auto STRING_JOIN( __scoped_renderer_popper_, __LINE__ ) = \
      renderer.push_mods( [&]( rr::RendererMods& mods ) {   \
        if( !mods.leaf_path.has_value() )                   \
          mods.leaf_path = false;                           \
        *mods.leaf_path = *mods.leaf_path || __VA_ARGS__;   \
      } );

/****************************************************************
** RendererConfig
*****************************************************************/
struct RendererConfig {
  gfx::size                             logical_screen_size = {};
  gfx::size                             max_atlas_size      = {};
  std::vector<SpriteSheetConfig> const& sprite_sheets;
  std::vector<AsciiFontSheetConfig> const& font_sheets;
};

/****************************************************************
** RendererMods
*****************************************************************/
struct BufferInfo {
  e_render_buffer buffer = e_render_buffer::normal;
};

struct RendererMods {
  PainterMods painter_mods = {};
  BufferInfo  buffer_mods  = {};
};

template<typename Func>
concept ModEditFunc =
    std::is_invocable_r_v<void, Func, RendererMods&>;

/****************************************************************
** Renderer
*****************************************************************/
// The video driver must have been fully initialized before using
// this.
struct Renderer {
  // The renderer must take ownership of this function.
  using PresentFn = std::function<void()>;

  // Return a unique_ptr because the Renderer is neither copyable
  // nor movable, and so that makes it easier to work with.
  static std::unique_ptr<Renderer> create(
      RendererConfig const& config, PresentFn present_fn );

  ~Renderer() noexcept;

  // Must be called each time the logical screen size changes.
  void set_logical_screen_size( gfx::size new_size );

  gfx::size logical_screen_size() const;

  // Must be called each time the logical screen size changes.
  void set_physical_screen_size( gfx::size new_size );

  void clear_screen( gfx::pixel color = gfx::pixel::black() );

  void set_color_cycle_stage( int stage );

  void set_camera( gfx::dsize translation, double zoom );

  // This is the one to call to do a full render pass; it
  //
  //   1. Calls begin_pass.
  //   2. Clears the buffer to black.
  //   3. Calls your function with *this.
  //   4. Calls end_pass.
  //   5. Presents.
  //
  // It takes the function that does the drawing.
  void render_pass(
      base::function_ref<void( Renderer& )> drawer );

  void clear_buffer( e_render_buffer buffer );
  void render_buffer( e_render_buffer buffer );

  // If the buffer is not specified then use the current one.
  long buffer_vertex_cur_pos(
      base::maybe<e_render_buffer> buffer = base::nothing );

  long   buffer_vertex_count( e_render_buffer buffer );
  double buffer_size_mb( e_render_buffer buffer );

  // Will run the function and return the range corresponding to
  // the vertices that were added in this function. Note that
  // this only works if the function writes to the current buffer
  // only.
  VertexRange range_for( base::function_ref<void()> f ) const;

  // This will edit the vertex buffer to zero-out all vertices
  // from [start, end). The GenericVertex is set up so that when
  // it is zero'd its `visible` field will be false (0) which
  // will cause the vertex shader to discard it, thus it won't
  // take any (expensive) fragment shader time. This is used when
  // we want to overwrite a tile with another tile and we don't
  // want the fragment shader to bother running on the vertices
  // from the previous tile for performance reasons. Once the
  // vertex buffer is edited that subsection (only) will be
  // re-uploaded to the GPU.
  void zap( VertexRange const& rng );

  Painter painter();

  Typer typer( gfx::point start, gfx::pixel color );

  Typer typer( std::string_view font_name, gfx::point start,
               gfx::pixel color );

  Typer typer( std::string_view font_name, gfx::point start,
               gfx::pixel color, Painter const& painter );

  gfx::size atlas_img_size() const;

  // Given a globally unique name for a texture in the atlas,
  // this will return its id for use when rendering it.
  std::unordered_map<std::string_view, int> const& atlas_ids()
      const;

  void present();

  RendererMods const& mods() const;

 private:
  // Mods.

  void mods_push_back( RendererMods&& mods );
  void mods_pop();

  struct [[nodiscard]] mod_popper {
    mod_popper( Renderer& r ) : r_( r ) {}
    ~mod_popper() noexcept { r_.mods_pop(); }
    NO_COPY_NO_MOVE( mod_popper );

   private:
    Renderer& r_;
  };

 public:
  template<ModEditFunc Func>
  mod_popper push_mods( Func&& func ) {
    RendererMods new_mods = mods();
    func( new_mods );
    mods_push_back( std::move( new_mods ) );
    return mod_popper{ *this };
  }

 private:
  // This must be called before any other rendering methods that
  // might generate vertices.
  void begin_pass();

  // This will end the rendering pass and upload the vertex data
  // to the GPU.  Returns the number of vertices uploaded.
  int end_pass();

  NO_COPY_NO_MOVE( Renderer );
  struct Impl;

  Renderer( Impl* impl );

  // Can't use unique_ptr here because it needs visibility into
  // the destructor of Impl.
  Impl* impl_ = nullptr;
};

} // namespace rr
