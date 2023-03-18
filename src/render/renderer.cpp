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
#include "refl/enum-map.hpp"
#include "refl/query-enum.hpp"
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
** RenderBuffer
*****************************************************************/
struct RenderBuffer {
  VertexArray_t vertex_array = {};
  // This is a pointer to keep its location stable since the
  // emitter needs to refer to it, and we don't want to make
  // this struct immovable.
  unique_ptr<vector<GenericVertex>> vertices = {};
  Emitter                           emitter;
  // If this is false then it will be assumed dirty always and
  // rerendered on every frame. Otherwise, it will only be ren-
  // dered when dirty, after which the dirty flag will be set to
  // false.
  bool track_dirty = false;
  bool dirty       = true;
};

/****************************************************************
** Renderer::Impl
*****************************************************************/
struct Renderer::Impl {
  // We use a maybe for the value to make this default con-
  // structible and so that when we do default construct it it
  // won't start making OpenGL calls to initialize the vertex ar-
  // ray.
  using RenderBufferMap =
      refl::enum_map<e_render_buffer, base::maybe<RenderBuffer>>;

  Impl( PresentFn present_fn_arg, ProgramType program_arg,
        RenderBufferMap buffers_arg, AtlasMap atlas_map_arg,
        size atlas_size_arg, gl::Texture atlas_tx_arg,
        unordered_map<string, int>       atlas_ids_arg,
        unordered_map<string_view, int>  atlas_ids_fast_arg,
        unordered_map<string, AsciiFont> ascii_fonts_arg,
        unordered_map<string_view, AsciiFont*>
                  ascii_fonts_fast_arg,
        gfx::size logical_screen_size_arg )
    : mod_stack{},
      present_fn( std::move( present_fn_arg ) ),
      program( std::move( program_arg ) ),
      buffers( std::move( buffers_arg ) ),
      atlas_map( std::move( atlas_map_arg ) ),
      atlas_size( std::move( atlas_size_arg ) ),
      atlas_tx( std::move( atlas_tx_arg ) ),
      atlas_tx_binder( atlas_tx.bind() ),
      atlas_ids( std::move( atlas_ids_arg ) ),
      atlas_ids_fast( std::move( atlas_ids_fast_arg ) ),
      ascii_fonts( std::move( ascii_fonts_arg ) ),
      ascii_fonts_fast( std::move( ascii_fonts_fast_arg ) ),
      logical_screen_size( logical_screen_size_arg ) {
    mod_stack.push( RendererMods{} );
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

    RenderBufferMap buffers;
    for( e_render_buffer const buffer :
         refl::enum_values<e_render_buffer> ) {
      auto       vertices = make_unique<vector<GenericVertex>>();
      auto*      p_vertices = vertices.get();
      bool const track_dirty =
          ( buffer == e_render_buffer::landscape ) ||
          ( buffer == e_render_buffer::landscape_annex );
      fmt::print(
          "initializing render buffer: {} with "
          "track_dirty={}.\n",
          buffer, track_dirty );
      buffers[buffer] =
          RenderBuffer{ .vertex_array = {},
                        .vertices     = std::move( vertices ),
                        .emitter      = Emitter( *p_vertices ),
                        .track_dirty  = track_dirty,
                        .dirty        = true };
    }

    auto pgrm = [&] {
      // Some OpenGL drivers, during shader program validation,
      // seem to require a vertex array to be bound to include in
      // the validation process.
      auto va_binder =
          buffers[e_render_buffer::normal]->vertex_array.bind();
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
        /*buffers=*/std::move( buffers ),
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
    for( auto& [buffer, data] : buffers ) {
      // This will prevent resetting the buffers for e.g. the
      // landscape buffers which don't get redrawn each frame.
      if( data->track_dirty ) continue;
      auto& vertices = *buffers[buffer]->vertices;
      auto& emitter  = buffers[buffer]->emitter;
      [[maybe_unused]] size_t capacity_before_clear =
          vertices.capacity();
      vertices.clear();
      DCHECK( vertices.capacity() == capacity_before_clear );
      DCHECK( vertices.empty() );
      emitter.set_position( 0 );
    }
  }

  int end_pass() {
    render_buffer( e_render_buffer::normal );
    auto& vertices = *buffers[e_render_buffer::normal]->vertices;
    return vertices.size();
  }

  Emitter& curr_emitter() {
    return buffers[mods().buffer_mods.buffer]->emitter;
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
    for( auto& [buffer, data] : buffers )
      if( data->track_dirty ) data->dirty = true;
  }

  void mods_pop() {
    DCHECK( mod_stack.size() > 1 );
    mod_stack.pop();
  }

  void clear_buffer( e_render_buffer buffer ) {
    buffers[buffer]->vertices->clear();
    buffers[buffer]->emitter.set_position( 0 );
  }

  long buffer_vertex_cur_pos(
      base::maybe<e_render_buffer> buffer = base::nothing ) {
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

  vector<GenericVertex>& get_buffer( e_render_buffer buffer ) {
    return *buffers[buffer]->vertices;
  }

  Emitter& get_emitter( e_render_buffer buffer ) {
    return buffers[buffer]->emitter;
  }

  VertexArray_t const& get_vertex_array(
      e_render_buffer buffer ) {
    return buffers[buffer]->vertex_array;
  }

  bool is_buffer_dirty( e_render_buffer buffer ) {
    return buffers[buffer]->dirty;
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

  void render_buffer( e_render_buffer buffer ) {
    auto const& vertex_array = buffers[buffer]->vertex_array;
    auto&       vertices     = *buffers[buffer]->vertices;
    bool&       dirty        = buffers[buffer]->dirty;
    if( !buffers[buffer]->track_dirty || dirty )
      vertex_array.buffer<0>().upload_data_replace(
          vertices, gl::e_draw_mode::stat1c );
    dirty = false;
    // Still need to run even if it wasn't dirty because uniforms
    // may have changed.
    program.run( vertex_array, vertices.size() );
  }

  // TODO: we probably don't need to keep the mods in a std::s-
  // tack, instead we can keep them in the popper object since
  // those will be stored on the stack and popped in reverse
  // order as they were applied, which should do the same job.
  stack<RendererMods>                          mod_stack;
  PresentFn                                    present_fn;
  ProgramType                                  program;
  RenderBufferMap                              buffers;
  AtlasMap const                               atlas_map;
  size const                                   atlas_size;
  gl::Texture const                            atlas_tx;
  TextureBinder                                atlas_tx_binder;
  unordered_map<string, int> const             atlas_ids;
  unordered_map<string_view, int> const        atlas_ids_fast;
  unordered_map<string, AsciiFont> const       ascii_fonts;
  unordered_map<string_view, AsciiFont*> const ascii_fonts_fast;
  gfx::size logical_screen_size;
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

void Renderer::clear_buffer( e_render_buffer buffer ) {
  impl_->clear_buffer( buffer );
}

void Renderer::render_buffer( e_render_buffer buffer ) {
  impl_->render_buffer( buffer );
}

long Renderer::buffer_vertex_cur_pos(
    base::maybe<e_render_buffer> buffer ) {
  return impl_->buffer_vertex_cur_pos( buffer );
}

long Renderer::buffer_vertex_count( e_render_buffer buffer ) {
  return impl_->buffers[buffer]->vertices->size();
}

double Renderer::buffer_size_mb( e_render_buffer buffer ) {
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
