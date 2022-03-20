/****************************************************************
**renderer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-12.
*
* Description: Top-level generic (mostly game-independent) 2D
*              renderer.
*
*****************************************************************/
#include "renderer.hpp"

// render
#include "ascii-font.hpp"
#include "atlas.hpp"
#include "emitter.hpp"
#include "misc.hpp"
#include "painter.hpp"
#include "sprite-sheet.hpp"
#include "typer.hpp"
#include "vertex.hpp"

// gl
#include "gl/iface.hpp"
#include "gl/shader.hpp"
#include "gl/texture.hpp"
#include "gl/uniform.hpp"
#include "gl/vertex-array.hpp"
#include "gl/vertex-buffer.hpp"

// refl
#include "refl/query-struct.hpp"
#include "refl/to-str.hpp"

// base
#include "base/fs.hpp"
#include "base/io.hpp"
#include "base/keyval.hpp"

// C++ standard library
#include <stack>

using namespace std;

namespace rr {

namespace {

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

using TextureBinder =
    decltype( std::declval<gl::Texture>().bind() );

/****************************************************************
** Shader Program Spec.
*****************************************************************/
using ProgramAttributes =
    refl::member_type_list_t<GenericVertex>;

struct ProgramUniforms {
  static constexpr tuple uniforms{
      gl::UniformSpec<int>( "u_atlas" ),
      gl::UniformSpec<gl::vec2>( "u_atlas_size" ),
      gl::UniformSpec<gl::vec2>( "u_screen_size" ),
  };
};

using ProgramType =
    gl::Program<ProgramAttributes, ProgramUniforms>;

/****************************************************************
** Vertex Array Spec.
*****************************************************************/
using VertexArray_t =
    gl::VertexArray<gl::VertexBuffer<GenericVertex>>;

} // namespace

/****************************************************************
** Renderer::Impl
*****************************************************************/
struct Renderer::Impl {
  Impl( PresentFn present_fn_arg, size logical_screen_size_arg,
        ProgramType program_arg, VertexArray_t vertex_array_arg,
        AtlasMap atlas_map_arg, size atlas_size_arg,
        gl::Texture                      atlas_tx_arg,
        unordered_map<string, int>       atlas_ids_arg,
        unordered_map<string_view, int>  atlas_ids_fast_arg,
        unordered_map<string, AsciiFont> ascii_fonts_arg,
        unordered_map<string_view, AsciiFont*>
            ascii_fonts_fast_arg )
    : mod_stack{},
      present_fn( std::move( present_fn_arg ) ),
      logical_screen_size( logical_screen_size_arg ),
      program( std::move( program_arg ) ),
      vertex_array( std::move( vertex_array_arg ) ),
      atlas_map( std::move( atlas_map_arg ) ),
      atlas_size( std::move( atlas_size_arg ) ),
      atlas_tx( std::move( atlas_tx_arg ) ),
      atlas_tx_binder( atlas_tx.bind() ),
      atlas_ids( std::move( atlas_ids_arg ) ),
      atlas_ids_fast( std::move( atlas_ids_fast_arg ) ),
      ascii_fonts( std::move( ascii_fonts_arg ) ),
      ascii_fonts_fast( std::move( ascii_fonts_fast_arg ) ),
      vertices{},
      emitter( vertices ) {
    mod_stack.push( RendererMods{} );
  };

  static Impl* create( RendererConfig const& config,
                       PresentFn             present_fn ) {
    using namespace ::base::literals;
    fs::path shaders = "src/render";

    UNWRAP_CHECK( vertex_shader_source,
                  base::read_text_file_as_string(
                      shaders / "generic.vert" ) );
    UNWRAP_CHECK( fragment_shader_source,
                  base::read_text_file_as_string(
                      shaders / "generic.frag" ) );
    UNWRAP_CHECK( vert_shader,
                  gl::Shader::create( gl::e_shader_type::vertex,
                                      vertex_shader_source ) );
    UNWRAP_CHECK( frag_shader, gl::Shader::create(
                                   gl::e_shader_type::fragment,
                                   fragment_shader_source ) );
    UNWRAP_CHECK(
        pgrm, ProgramType::create( vert_shader, frag_shader ) );

    pgrm["u_atlas"_t] = 0; // GL_TEXTURE0

    size logical_screen_size = config.logical_screen_size;
    pgrm["u_screen_size"_t] =
        gl::vec2::from_size( logical_screen_size );

    AtlasBuilder               atlas_builder;
    unordered_map<string, int> atlas_ids;

    for( SpriteSheetConfig const& sheet :
         config.sprite_sheets ) {
      CHECK_HAS_VALUE(
          load_sprite_sheet( atlas_builder, sheet, atlas_ids ) );
    }

    unordered_map<string, AsciiFont> ascii_fonts;
    for( AsciiFontSheetConfig const& sheet :
         config.font_sheets ) {
      UNWRAP_CHECK( ascii_font, load_ascii_font_sheet(
                                    atlas_builder, sheet ) );
      ascii_fonts.emplace( sheet.font_name,
                           std::move( ascii_font ) );
    }

    // Note: these maps are for speed since they will not require
    // creating strings for each lookup (at least until we get
    // heterogeneous lookup in standard containers), but they do
    // reference the keys in their associated std::string maps,
    // so those maps should not be changed after constructing
    // this fast version.
    unordered_map<string_view, int>        atlas_ids_fast;
    unordered_map<string_view, AsciiFont*> ascii_fonts_fast;
    for( auto const& [name, id] : atlas_ids )
      atlas_ids_fast[name] = id;
    for( auto& [name, ascii_font] : ascii_fonts )
      ascii_fonts_fast[name] = &ascii_font;

    // If the below line check-fails then you probably need to
    // increase the max texture atlas size.
    UNWRAP_CHECK_MSG(
        atlas, atlas_builder.build( config.max_atlas_size ),
        "failed to build texture atlas of maximum size {}.  You "
        "may need to increase the maximum size.",
        config.max_atlas_size );

    size        atlas_size = atlas.img.size_pixels();
    gl::Texture atlas_tx( std::move( atlas.img ) );

    pgrm["u_atlas_size"_t] = gl::vec2::from_size( atlas_size );

    // Note some fields are not explicitly initialized here
    // (there are initialized in the constructor above).
    return new Impl(
        /*present_fn=*/std::move( present_fn ),
        /*logical_screen_size=*/logical_screen_size,
        /*program=*/std::move( pgrm ),
        /*vertex_array=*/{},
        /*atlas_map=*/std::move( atlas.dict ),
        /*atlas_size=*/atlas_size,
        /*atlas_tx=*/std::move( atlas_tx ),
        /*atlas_ids=*/std::move( atlas_ids ),
        /*atlas_ids_fast=*/std::move( atlas_ids_fast ),
        /*ascii_fonts=*/std::move( ascii_fonts ),
        /*ascii_fonts_fast=*/std::move( ascii_fonts_fast ) );
  }

  void begin_pass() {
    // This should not affect the capacity.
    [[maybe_unused]] size_t capacity_before_clear =
        vertices.capacity();
    vertices.clear();
    DCHECK( vertices.capacity() == capacity_before_clear );
    DCHECK( vertices.empty() );
  }

  int end_pass() {
    int num_vertices = vertices.size();
    // Upload the vertices to the GPU.
    vertex_array.buffer<0>().upload_data_replace(
        vertices, gl::e_draw_mode::stat1c );
    using namespace ::base::literals;
    program["u_screen_size"_t] =
        gl::vec2::from_size( logical_screen_size );
    program.run( vertex_array, num_vertices );

    return num_vertices;
  }

  Painter painter() {
    return Painter( atlas_map, emitter,
                    mod_stack.top().painter_mods );
  }

  Typer typer( string_view font_name, point start, pixel color,
               Painter const& painter ) {
    UNWRAP_CHECK( p_ascii_font,
                  base::lookup( ascii_fonts_fast, font_name ) );
    return Typer( painter, *p_ascii_font, start, color );
  }

  Typer typer( string_view font_name, point start,
               pixel color ) {
    UNWRAP_CHECK( p_ascii_font,
                  base::lookup( ascii_fonts_fast, font_name ) );
    return Typer( painter(), *p_ascii_font, start, color );
  }

  void clear_screen( gfx::pixel color ) { gl::clear( color ); }

  void set_logical_screen_size( size new_size ) {
    logical_screen_size = new_size;
  }

  unordered_map<string_view, int> const& atlas_ids_fn() const {
    return atlas_ids_fast;
  }

  RendererMods const& mods() const {
    DCHECK( !mod_stack.empty() );
    return mod_stack.top();
  }

  void mods_push_back( RendererMods&& mods ) {
    mod_stack.push( std::move( mods ) );
  }

  void mods_pop() {
    DCHECK( mod_stack.size() > 1 );
    mod_stack.pop();
  }

  stack<RendererMods>                    mod_stack;
  PresentFn                              present_fn;
  size                                   logical_screen_size;
  ProgramType                            program;
  VertexArray_t const                    vertex_array;
  AtlasMap const                         atlas_map;
  size const                             atlas_size;
  gl::Texture const                      atlas_tx;
  TextureBinder                          atlas_tx_binder;
  unordered_map<string, int> const       atlas_ids;
  unordered_map<string_view, int> const  atlas_ids_fast;
  unordered_map<string, AsciiFont> const ascii_fonts;
  unordered_map<string_view, AsciiFont*> const ascii_fonts_fast;
  vector<GenericVertex>                        vertices;
  Emitter                                      emitter;
};

/****************************************************************
** Renderer
*****************************************************************/
Renderer::Renderer( Impl* impl ) : impl_( std::move( impl ) ) {
  CHECK( impl_ );
}

Renderer::~Renderer() noexcept {
  CHECK( impl_ );
  delete impl_;
}

void Renderer::begin_pass() { impl_->begin_pass(); }

int Renderer::end_pass() { return impl_->end_pass(); }

Painter Renderer::painter() { return impl_->painter(); }

Typer Renderer::typer( point start, pixel color ) {
  return impl_->typer( "simple", start, color );
}

Typer Renderer::typer( string_view font_name, point start,
                       pixel color ) {
  return impl_->typer( font_name, start, color );
}

Typer Renderer::typer( string_view font_name, point start,
                       pixel color, Painter const& painter ) {
  return impl_->typer( font_name, start, color, painter );
}

void Renderer::clear_screen( gfx::pixel color ) {
  impl_->clear_screen( color );
}

void Renderer::present() { impl_->present_fn(); }

void Renderer::set_logical_screen_size( gfx::size new_size ) {
  impl_->set_logical_screen_size( new_size );
}

unordered_map<string_view, int> const& Renderer::atlas_ids()
    const {
  return impl_->atlas_ids_fn();
}

unique_ptr<Renderer> Renderer::create(
    RendererConfig const& config, PresentFn present_fn ) {
  return unique_ptr<Renderer>( new Renderer(
      Impl::create( config, std::move( present_fn ) ) ) );
}

RendererMods const& Renderer::mods() const {
  return impl_->mods();
}

void Renderer::mods_push_back( RendererMods&& mods ) {
  impl_->mods_push_back( std::move( mods ) );
}

void Renderer::mods_pop() { impl_->mods_pop(); }

gfx::size Renderer::atlas_img_size() const {
  return impl_->atlas_size;
}

} // namespace rr
