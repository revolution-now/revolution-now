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
#include "base/function-ref.hpp"
#include "base/macros.hpp"

// C++ standard library
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace rr {

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
struct RendererMods {
  PainterMods painter_mods = {};
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

  void clear_screen( gfx::pixel color = gfx::pixel::black() );

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

  // This must be called before any other rendering methods that
  // might generate vertices.
  void begin_pass();

  // This will end the rendering pass and upload the vertex data
  // to the GPU.  Returns the number of vertices uploaded.
  int end_pass();

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
  NO_COPY_NO_MOVE( Renderer );
  struct Impl;

  Renderer( Impl* impl );

  // Can't use unique_ptr here because it needs visibility into
  // the destructor of Impl.
  Impl* impl_ = nullptr;
};

} // namespace rr
