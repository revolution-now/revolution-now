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

using namespace ::std;
using namespace ::base::literals;

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
      gl::UniformSpec<int32_t>( "u_color_cycle_stage" ),
      gl::UniformSpec<gl::vec2>( "u_camera_translation" ),
      gl::UniformSpec<float>( "u_camera_zoom" ),
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
  Impl( PresentFn present_fn_arg, ProgramType program_arg,
        VertexArray_t vertex_array_arg,
        VertexArray_t landscape_vertex_array_arg,
        VertexArray_t landscape_annex_vertex_array_arg,
        VertexArray_t backdrop_vertex_array_arg,
        AtlasMap atlas_map_arg, size atlas_size_arg,
        gl::Texture                      atlas_tx_arg,
        unordered_map<string, int>       atlas_ids_arg,
        unordered_map<string_view, int>  atlas_ids_fast_arg,
        unordered_map<string, AsciiFont> ascii_fonts_arg,
        unordered_map<string_view, AsciiFont*>
                  ascii_fonts_fast_arg,
        gfx::size logical_screen_size_arg )
    : mod_stack{},
      present_fn( std::move( present_fn_arg ) ),
      program( std::move( program_arg ) ),
      vertex_array( std::move( vertex_array_arg ) ),
      landscape_vertex_array(
          std::move( landscape_vertex_array_arg ) ),
      landscape_annex_vertex_array(
          std::move( landscape_annex_vertex_array_arg ) ),
      backdrop_vertex_array(
          std::move( backdrop_vertex_array_arg ) ),
      atlas_map( std::move( atlas_map_arg ) ),
      atlas_size( std::move( atlas_size_arg ) ),
      atlas_tx( std::move( atlas_tx_arg ) ),
      atlas_tx_binder( atlas_tx.bind() ),
      atlas_ids( std::move( atlas_ids_arg ) ),
      atlas_ids_fast( std::move( atlas_ids_fast_arg ) ),
      ascii_fonts( std::move( ascii_fonts_arg ) ),
      ascii_fonts_fast( std::move( ascii_fonts_fast_arg ) ),
      vertices{},
      landscape_vertices{},
      landscape_annex_vertices{},
      backdrop_vertices{},
      emitter( vertices ),
      landscape_emitter( landscape_vertices ),
      landscape_annex_emitter( landscape_annex_vertices ),
      backdrop_emitter( backdrop_vertices ),
      logical_screen_size( logical_screen_size_arg ),
      landscape_dirty( true ),
      landscape_annex_dirty( true ) {
    mod_stack.push( RendererMods{} );
    emitter.log_capacity_changes( false );
    landscape_emitter.log_capacity_changes( false );
    landscape_annex_emitter.log_capacity_changes( false );
    backdrop_emitter.log_capacity_changes( false );
  };

  static Impl* create( RendererConfig const& config,
                       PresentFn             present_fn ) {
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

    gl::VertexArray<gl::VertexBuffer<GenericVertex>>
        vertex_array;
    gl::VertexArray<gl::VertexBuffer<GenericVertex>>
        landscape_vertex_array;
    gl::VertexArray<gl::VertexBuffer<GenericVertex>>
        landscape_annex_vertex_array;
    gl::VertexArray<gl::VertexBuffer<GenericVertex>>
         backdrop_vertex_array;
    auto pgrm = [&] {
      // Some OpenGL drivers, during shader program validation,
      // seem to require a vertex array to be bound to include in
      // the validation process.
      auto va_binder = vertex_array.bind();
      UNWRAP_CHECK( pgrm, ProgramType::create( vert_shader,
                                               frag_shader ) );
      return std::move( pgrm );
    }();

    pgrm["u_atlas"_t] = 0; // GL_TEXTURE0

    gfx::size logical_screen_size = config.logical_screen_size;
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
        /*program=*/std::move( pgrm ),
        /*vertex_array=*/std::move( vertex_array ),
        /*landscape_vertex_array=*/
        std::move( landscape_vertex_array ),
        /*landscape_annex_vertex_array=*/
        std::move( landscape_annex_vertex_array ),
        /*backdrop_vertex_array=*/
        std::move( backdrop_vertex_array ),
        /*atlas_map=*/std::move( atlas.dict ),
        /*atlas_size=*/atlas_size,
        /*atlas_tx=*/std::move( atlas_tx ),
        /*atlas_ids=*/std::move( atlas_ids ),
        /*atlas_ids_fast=*/std::move( atlas_ids_fast ),
        /*ascii_fonts=*/std::move( ascii_fonts ),
        /*ascii_fonts_fast=*/std::move( ascii_fonts_fast ),
        /*logical_screen_size=*/logical_screen_size );
  }

  void begin_pass() {
    // This should not affect the capacity.
    {
      [[maybe_unused]] size_t capacity_before_clear =
          vertices.capacity();
      vertices.clear();
      DCHECK( vertices.capacity() == capacity_before_clear );
      DCHECK( vertices.empty() );
      emitter.set_position( 0 );
    }
    {
      [[maybe_unused]] size_t capacity_before_clear =
          backdrop_vertices.capacity();
      backdrop_vertices.clear();
      DCHECK( backdrop_vertices.capacity() ==
              capacity_before_clear );
      DCHECK( backdrop_vertices.empty() );
      backdrop_emitter.set_position( 0 );
    }
    // We don't reset the position of the landscape emitters.
  }

  int end_pass() {
    render_buffer( e_render_target_buffer::normal );
    return vertices.size();
  }

  Emitter& curr_emitter() {
    Emitter* modded_emitter = nullptr;
    switch( mods().buffer_mods.buffer ) {
      case e_render_target_buffer::normal:
        modded_emitter = &emitter;
        break;
      case e_render_target_buffer::landscape:
        modded_emitter = &landscape_emitter;
        break;
      case e_render_target_buffer::landscape_annex:
        modded_emitter = &landscape_annex_emitter;
        break;
      case e_render_target_buffer::backdrop:
        modded_emitter = &backdrop_emitter;
        break;
    }
    return *modded_emitter;
  }

  Painter painter() {
    return Painter( atlas_map, curr_emitter(),
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
    program["u_screen_size"_t] = gl::vec2::from_size( new_size );
    logical_screen_size        = new_size;
  }

  void set_physical_screen_size( size new_size ) {
    gl::set_viewport( rect{ .origin = {}, .size = new_size } );
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
    if( mods.buffer_mods.buffer ==
        e_render_target_buffer::landscape )
      landscape_dirty = true;
    if( mods.buffer_mods.buffer ==
        e_render_target_buffer::landscape_annex )
      landscape_annex_dirty = true;
  }

  void mods_pop() {
    DCHECK( mod_stack.size() > 1 );
    mod_stack.pop();
  }

  void clear_buffer( e_render_target_buffer buffer ) {
    switch( buffer ) {
      case e_render_target_buffer::normal:
        // We don't currently have a use for this, because the
        // normal buffer gets automatically cleared at the start
        // of each render pass.
        SHOULD_NOT_BE_HERE;
      case e_render_target_buffer::landscape:
        landscape_vertices.clear();
        landscape_emitter.set_position( 0 );
        break;
      case e_render_target_buffer::landscape_annex:
        landscape_annex_vertices.clear();
        landscape_annex_emitter.set_position( 0 );
        break;
      case e_render_target_buffer::backdrop:
        // We don't currently have a use for this, because the
        // backdrop buffer gets automatically cleared at the
        // start of each render pass.
        SHOULD_NOT_BE_HERE;
    }
  }

  long buffer_vertex_cur_pos( base::maybe<e_render_target_buffer>
                                  buffer = base::nothing ) {
    return get_emitter(
               buffer.value_or( mods().buffer_mods.buffer ) )
        .position();
  }

  VertexRange range_for( base::function_ref<void()> f ) {
    VertexRange rng;
    rng.buffer = mods().buffer_mods.buffer;
    rng.start  = buffer_vertex_cur_pos();
    f();
    rng.finish = buffer_vertex_cur_pos();
    return rng;
  }

  vector<GenericVertex>& get_buffer(
      e_render_target_buffer buffer ) {
    switch( buffer ) {
      case e_render_target_buffer::normal: return vertices;
      case e_render_target_buffer::landscape:
        return landscape_vertices;
      case e_render_target_buffer::landscape_annex:
        return landscape_annex_vertices;
      case e_render_target_buffer::backdrop:
        return backdrop_vertices;
    }
  }

  Emitter& get_emitter( e_render_target_buffer buffer ) {
    switch( buffer ) {
      case e_render_target_buffer::normal: return emitter;
      case e_render_target_buffer::landscape:
        return landscape_emitter;
      case e_render_target_buffer::landscape_annex:
        return landscape_annex_emitter;
      case e_render_target_buffer::backdrop:
        return backdrop_emitter;
    }
  }

  VertexArray_t const& get_vertex_array(
      e_render_target_buffer buffer ) {
    switch( buffer ) {
      case e_render_target_buffer::normal: return vertex_array;
      case e_render_target_buffer::landscape:
        return landscape_vertex_array;
      case e_render_target_buffer::landscape_annex:
        return landscape_annex_vertex_array;
      case e_render_target_buffer::backdrop:
        return backdrop_vertex_array;
    }
  }

  bool is_buffer_dirty( e_render_target_buffer buffer ) {
    switch( buffer ) {
      case e_render_target_buffer::normal:
        // This is equivalent to always being dirty because it is
        // reuploaded to the GPU each frame.
        return true;
      case e_render_target_buffer::landscape:
        return landscape_dirty;
      case e_render_target_buffer::landscape_annex:
        return landscape_annex_dirty;
      case e_render_target_buffer::backdrop:
        // This is equivalent to always being dirty because it is
        // reuploaded to the GPU each frame.
        return true;
    }
  }

  void zap( VertexRange const& rng ) {
    CHECK_GE( rng.finish, rng.start );
    if( rng.finish == rng.start ) return;
    vector<GenericVertex>& vertices = get_buffer( rng.buffer );
    CHECK_GT( int( vertices.size() ), rng.start );
    CHECK_LE( rng.finish, int( vertices.size() ) );
    auto start_iter = vertices.begin() + rng.start;
    auto end_iter   = vertices.begin() + rng.finish;

    // zero it out.
    fill( start_iter, end_iter, GenericVertex{} );

    // If the buffer is dirty then that means that the buffer on
    // the GPU may not correspond to the vertex array and so it
    // is not safe to modify a subsection of it. That's ok, be-
    // cause if it is dirty then the entire buffer will get
    // re-uploaded to the GPU anyway at the end of the next
    // render pass.
    if( !is_buffer_dirty( rng.buffer ) ) {
      // Re-upload only this segment to the GPU.
      span const           segment{ start_iter, end_iter };
      VertexArray_t const& vertex_array =
          get_vertex_array( rng.buffer );
      vertex_array.buffer<0>().upload_data_modify( segment,
                                                   rng.start );
    }
  }

  void render_buffer( e_render_target_buffer buffer ) {
    switch( buffer ) {
      case e_render_target_buffer::backdrop: {
        vertex_array.buffer<0>().upload_data_replace(
            backdrop_vertices, gl::e_draw_mode::stat1c );
        program.run( vertex_array, backdrop_vertices.size() );
        break;
      }
      case e_render_target_buffer::normal: {
        vertex_array.buffer<0>().upload_data_replace(
            vertices, gl::e_draw_mode::stat1c );
        program.run( vertex_array, vertices.size() );
        break;
      }
      case e_render_target_buffer::landscape: {
        if( landscape_dirty ) {
          landscape_vertex_array.buffer<0>().upload_data_replace(
              landscape_vertices, gl::e_draw_mode::stat1c );
          landscape_dirty = false;
        }
        // Still need to run even if landscape has not been modi-
        // fied because the camera uniforms may have changed.
        program.run( landscape_vertex_array,
                     landscape_vertices.size() );
        break;
      }
      case e_render_target_buffer::landscape_annex: {
        if( landscape_annex_dirty ) {
          landscape_annex_vertex_array.buffer<0>()
              .upload_data_replace( landscape_annex_vertices,
                                    gl::e_draw_mode::stat1c );
          landscape_annex_dirty = false;
        }
        // Still need to run even if landscape_annex has not been
        // modified because the camera uniforms may have changed.
        program.run( landscape_annex_vertex_array,
                     landscape_annex_vertices.size() );
        break;
      }
    }
  }

  // TODO: we probably don't need to keep the mods in a std::s-
  // tack, instead we can keep them in the popper object since
  // those will be stored on the stack and popped in reverse
  // order as they were applied, which should do the same job.
  stack<RendererMods>              mod_stack;
  PresentFn                        present_fn;
  ProgramType                      program;
  VertexArray_t const              vertex_array;
  VertexArray_t const              landscape_vertex_array;
  VertexArray_t const              landscape_annex_vertex_array;
  VertexArray_t const              backdrop_vertex_array;
  AtlasMap const                   atlas_map;
  size const                       atlas_size;
  gl::Texture const                atlas_tx;
  TextureBinder                    atlas_tx_binder;
  unordered_map<string, int> const atlas_ids;
  unordered_map<string_view, int> const        atlas_ids_fast;
  unordered_map<string, AsciiFont> const       ascii_fonts;
  unordered_map<string_view, AsciiFont*> const ascii_fonts_fast;
  vector<GenericVertex>                        vertices;
  vector<GenericVertex> landscape_vertices;
  vector<GenericVertex> landscape_annex_vertices;
  vector<GenericVertex> backdrop_vertices;
  Emitter               emitter;
  Emitter               landscape_emitter;
  Emitter               landscape_annex_emitter;
  Emitter               backdrop_emitter;
  gfx::size             logical_screen_size;
  bool                  landscape_dirty;
  bool                  landscape_annex_dirty;
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

gfx::size Renderer::logical_screen_size() const {
  return impl_->logical_screen_size;
}

void Renderer::set_physical_screen_size( gfx::size new_size ) {
  impl_->set_physical_screen_size( new_size );
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

void Renderer::render_pass(
    base::function_ref<void( Renderer& )> drawer ) {
  begin_pass();
  drawer( *this );
  end_pass();
  present();
}

void Renderer::set_color_cycle_stage( int stage ) {
  impl_->program["u_color_cycle_stage"_t] = stage;
}

void Renderer::set_camera( gfx::dsize translation,
                           double     zoom ) {
  impl_->program["u_camera_translation"_t] =
      gl::vec2::from_dsize( translation );
  impl_->program["u_camera_zoom"_t] = zoom;
}

void Renderer::clear_buffer( e_render_target_buffer buffer ) {
  impl_->clear_buffer( buffer );
}

void Renderer::render_buffer( e_render_target_buffer buffer ) {
  impl_->render_buffer( buffer );
}

long Renderer::buffer_vertex_cur_pos(
    base::maybe<e_render_target_buffer> buffer ) {
  return impl_->buffer_vertex_cur_pos( buffer );
}

long Renderer::buffer_vertex_count(
    e_render_target_buffer buffer ) {
  switch( buffer ) {
    case e_render_target_buffer::backdrop:
      return impl_->backdrop_vertices.size();
    case e_render_target_buffer::normal:
      return impl_->vertices.size();
    case e_render_target_buffer::landscape:
      return impl_->landscape_vertices.size();
    case e_render_target_buffer::landscape_annex:
      return impl_->landscape_annex_vertices.size();
  }
}

double Renderer::buffer_size_mb(
    e_render_target_buffer buffer ) {
  return buffer_vertex_count( buffer ) *
         sizeof( GenericVertex ) / ( 1024.0 * 1024.0 );
}

void Renderer::zap( VertexRange const& rng ) {
  impl_->zap( rng );
}

VertexRange Renderer::range_for(
    base::function_ref<void()> f ) const {
  return impl_->range_for( f );
}

} // namespace rr
