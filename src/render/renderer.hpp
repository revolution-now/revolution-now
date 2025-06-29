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
#include "irenderer.hpp"
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

namespace gfx {
enum class e_resolution;
}

namespace rr {

struct RenderPassOpts {
  // Clear the buffers before rendering (except for those that
  // use dirty-tracking, such as the landscape buffer).
  //
  // WARNING: This should only ever be changed by very special-
  // ized code that needs to render something over whatever is
  // currently rendered, e.g. a diagnostic message. Normal ren-
  // dering code should not touch this, otherwise buffers run
  // away with memory and the game would OOM.
  bool clear_buffers = true;
};

/****************************************************************
** Macros
*****************************************************************/
// This makes it easier to do a scoped change of the renderer
// mods.  Use it like so:
//
// {
//  SCOPED_RENDERER_MOD_SET( painter_mods.repos.scale, 2.0 );
//  SCOPED_RENDERER_MOD_ADD( painter_mods.repos.translation2, 5);
//  SCOPED_RENDERER_MOD_MUL( painter_mods.repos.scale, 1.5 );
//  ...
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
  gfx::size logical_screen_size = {};
  gfx::size max_atlas_size      = {};
  std::vector<SpriteSheetConfig> const& sprite_sheets;
  std::vector<AsciiFontSheetConfig> const& font_sheets;
  base::maybe<std::string> dump_atlas_png    = {};
  base::maybe<std::string> dump_noise_png    = {};
  e_render_framebuffer_mode framebuffer_mode = {};
};

/****************************************************************
** RendererMods
*****************************************************************/
struct BufferInfo {
  e_render_buffer buffer = e_render_buffer::normal;
};

struct RendererMods {
  PainterMods painter_mods = {};
  BufferInfo buffer_mods   = {};
};

template<typename Func>
concept ModEditFunc =
    std::is_invocable_r_v<void, Func, RendererMods&>;

/****************************************************************
** Renderer
*****************************************************************/
// The video driver must have been fully initialized before using
// this.
struct Renderer : IRenderer, IRendererSettings {
  // The renderer must take ownership of this function.
  using PresentFn = std::function<void()>;

  // Return a unique_ptr because the Renderer is neither copyable
  // nor movable, and so that makes it easier to work with.
  static std::unique_ptr<Renderer> create(
      RendererConfig const& config, PresentFn present_fn );

  ~Renderer() noexcept override;

  // Must be called each time the logical screen size changes.
  void set_logical_screen_size( gfx::size new_size );

  gfx::size logical_screen_size() const;

  // For convenience.
  gfx::rect logical_screen_rect() const {
    return gfx::rect{ .origin = {},
                      .size   = logical_screen_size() };
  }

  // For convenience.
  gfx::e_resolution named_logical_resolution() const;

  // Must be called each time the logical screen size changes.
  void set_viewport( gfx::rect viewport );

  void clear_screen( gfx::pixel color = gfx::pixel::black() );

  void set_color_cycle_stage( int stage ) override;

  void set_color_cycle_plans(
      std::vector<gfx::pixel> const& plans ) override;

  void set_color_cycle_keys(
      std::span<gfx::pixel const> plans ) override;

  void set_uniform_depixelation_stage( double stage );

  void set_camera( gfx::dsize translation, double zoom );

  // This is the one to call to do a full render pass; it
  //
  //   1. Calls begin_pass.
  //   2. Clears the buffer to black (by default).
  //   3. Calls your function with *this.
  //   4. Calls end_pass.
  //   5. Presents.
  //
  // It takes the function that does the drawing.
  void render_pass(
      base::function_ref<void( Renderer& ) const> drawer,
      RenderPassOpts const& opts = {} );

  e_render_framebuffer_mode render_framebuffer_mode()
      const override;

  void set_render_framebuffer_mode(
      e_render_framebuffer_mode mode ) override;

  void clear_buffer( e_render_buffer buffer );

  // If the buffer is not specified then use the current one.
  long buffer_vertex_cur_pos(
      base::maybe<e_render_buffer> buffer = base::nothing );

  long buffer_vertex_count( e_render_buffer buffer );
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

  Typer typer();

  Typer typer( TextLayout const& layout );

  Typer typer( TextLayout const& layout, gfx::point start,
               gfx::pixel color );

  Typer typer( std::string_view font_name,
               TextLayout const& layout, gfx::point start,
               gfx::pixel color );

  Typer typer( std::string_view font_name,
               TextLayout const& layout, gfx::point start,
               gfx::pixel color, Painter const& painter );

  Typer typer( gfx::point start, gfx::pixel color );

  AtlasMap const& atlas() const;
  AsciiFont const& ascii_font(
      std::string_view font_name ) const;

  gfx::size atlas_img_size() const;

  // Given a globally unique name for a texture in the atlas,
  // this will return its id for use when rendering it.
  std::unordered_map<std::string_view, int> const& atlas_ids()
      const;

  // Given a globally unique name for a texture in the atlas,
  // this will return its trimmed rect.
  std::unordered_map<std::string, gfx::rect> const&
  atlas_trimmed_rects() const;

  void present();

  RendererMods const& mods() const;

  // Normally we shouldn't need to request that a specific buffer
  // get redrawn because that happens automatically at the end of
  // each render pass.
  void testing_only_render_buffer( e_render_buffer buffer );

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
  NO_COPY_NO_MOVE( Renderer );
  struct Impl;

  Renderer( Impl* impl );

  // Can't use unique_ptr here because it needs visibility into
  // the destructor of Impl.
  Impl* impl_ = nullptr;
};

} // namespace rr
