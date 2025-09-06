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
#include "noise.hpp"
#include "painter.hpp"
#include "sprite-sheet.hpp"
#include "text-layout.rds.hpp"
#include "typer.hpp"
#include "vertex.hpp"

// gl
#include "gl/framebuffer.hpp"
#include "gl/iface.hpp"
#include "gl/shader.hpp"
#include "gl/texture.hpp"
#include "gl/uniform.hpp"
#include "gl/vertex-array.hpp"
#include "gl/vertex-buffer.hpp"

// stb
#include "stb/image.hpp"

// gfx
#include "gfx/resolution-enum.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/query-enum.hpp"
#include "refl/query-struct.hpp"
#include "refl/to-str.hpp"

// base
#include "base/fs.hpp"
#include "base/io.hpp"
#include "base/keyval.hpp"
#include "base/logger.hpp"
#include "base/scope-exit.hpp"

// C++ standard library
#include <array>
#include <stack>

using namespace ::std;
using namespace ::base::literals;

namespace rr {

namespace {

using ::base::function_ref;
using ::base::lg;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

using TextureBinder =
    decltype( std::declval<gl::Texture>().bind() );

string_view constexpr kDefaultFontName = "simple";

/****************************************************************
** Shader Program Spec.
*****************************************************************/
using ProgramAttributes =
    refl::member_type_list_t<GenericVertex>;

struct NormalProgramUniforms {
  static constexpr tuple uniforms{
    gl::UniformSpec<int>( "u_atlas" ),
    gl::UniformSpec<int>( "u_noise" ),
    gl::UniformSpec<gl::vec2>( "u_atlas_size" ),
    gl::UniformSpec<gl::vec2>( "u_noise_size" ),
    gl::UniformSpec<gl::vec2>( "u_screen_size" ),
    gl::UniformSpec<int32_t>( "u_color_cycle_stage" ),
    gl::UniformSpec<gl::vec2>( "u_camera_translation" ),
    gl::UniformSpec<float>( "u_camera_zoom" ),
    gl::UniformSpec<float>( "u_depixelation_stage" ),
    gl::UniformSpec<span<gl::ivec4 const>>(
        "u_color_cycle_targets" ),
    gl::UniformSpec<span<gl::ivec3 const>>(
        "u_color_cycle_keys" ),
  };
};

struct PostProgramUniforms {
  static constexpr tuple uniforms{
    gl::UniformSpec<int>( "u_source" ),
    gl::UniformSpec<gl::vec2>( "u_screen_size" ),
  };
};

using NormalProgramType =
    gl::Program<ProgramAttributes, NormalProgramUniforms>;

using PostProgramType =
    gl::Program<ProgramAttributes, PostProgramUniforms>;

/****************************************************************
** Vertex Array Spec.
*****************************************************************/
using VertexArray_t =
    gl::VertexArray<gl::VertexBuffer<GenericVertex>>;

/****************************************************************
** Buffer info.
*****************************************************************/
e_render_buffer_phase buffer_phase(
    e_render_buffer const buffer ) {
  switch( buffer ) {
    case e_render_buffer::backdrop:
    case e_render_buffer::entities:
    case e_render_buffer::landscape:
    case e_render_buffer::landscape_anim_enpixelate:
    case e_render_buffer::landscape_anim_replace:
    case e_render_buffer::landscape_annex:
    case e_render_buffer::normal:
    case e_render_buffer::obfuscation:
    case e_render_buffer::obfuscation_annex:
      return e_render_buffer_phase::normal;

    case e_render_buffer::postprocessing:
      return e_render_buffer_phase::postprocessing;
  }
}

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
  Emitter emitter;
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

  Impl( PresentFn present_fn_arg, NormalProgramType program_arg,
        PostProgramType postprocessing_program_arg,
        RenderBufferMap buffers_arg, AtlasMap atlas_map_arg,
        size atlas_size_arg, gl::Texture atlas_tx_arg,
        gl::Texture noise_tx_arg,
        unordered_map<string, int> atlas_ids_arg,
        unordered_map<string_view, int> atlas_ids_fast_arg,
        unordered_map<string, gfx::rect> atlas_trimmed_rects_arg,
        unordered_map<int, int> atlas_burrow_ids_arg,
        unordered_map<string, AsciiFont> ascii_fonts_arg,
        unordered_map<string_view, AsciiFont*>
            ascii_fonts_fast_arg,
        gfx::size const logical_screen_size_arg,
        e_render_framebuffer_mode const framebuffer_mode_arg )
    : present_fn( std::move( present_fn_arg ) ),
      atlas_map( std::move( atlas_map_arg ) ),
      atlas_size( std::move( atlas_size_arg ) ),
      atlas_tx( std::move( atlas_tx_arg ) ),
      atlas_tx_binder( atlas_tx.bind() ),
      noise_tx( std::move( noise_tx_arg ) ),
      atlas_ids( std::move( atlas_ids_arg ) ),
      atlas_ids_fast( std::move( atlas_ids_fast_arg ) ),
      atlas_trimmed_rects(
          std::move( atlas_trimmed_rects_arg ) ),
      atlas_burrow_ids( std::move( atlas_burrow_ids_arg ) ),
      ascii_fonts( std::move( ascii_fonts_arg ) ),
      ascii_fonts_fast( std::move( ascii_fonts_fast_arg ) ),
      mod_stack{},
      normal_program( std::move( program_arg ) ),
      postprocessing_program(
          std::move( postprocessing_program_arg ) ),
      buffers( std::move( buffers_arg ) ) {
    mod_stack.push( RendererMods{} );

    framebuffer_mode_ = framebuffer_mode_arg;

    logical_screen_size = logical_screen_size_arg;
    recreate_postprocessing_framebuffer();

    // Bind the noise texture to texture 1. The reason we do a
    // permanent bind is because 1) we don't really need to ever
    // unbind this (or change the binding), at least at the mo-
    // ment, and 2) it would be a slight pain because we would
    // have to set the active texture again before unbinding
    // (restoring), which would require enhancing the binder API.
    {
      set_active_texture( gl::e_gl_texture::tx_1 );
      noise_tx.bind_permanent();
      set_active_texture( gl::e_gl_texture::tx_0 );
    }
  };

  void recreate_postprocessing_framebuffer() {
    CHECK( logical_screen_size.area() > 0 );
    // Ordering here is chosen carefully to destroy the frame-
    // buffer first (which refers to the texture) and to have
    // this work both on the initial creation and on re-creating.
    // render_framebuffer              = {};
    postprocessing_render_target_tx = {};
    postprocessing_render_target_tx.set_empty(
        logical_screen_size );
    render_framebuffer.set_color_attachment(
        postprocessing_render_target_tx );
    CHECK( render_framebuffer.is_framebuffer_complete() );
  }

  static Impl* create( RendererConfig const& config,
                       PresentFn present_fn ) {
    fs::path shaders = "src/render";

    // Main program.
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

    // Post processing program.
    UNWRAP_CHECK( postprocessing_vertex_shader_source,
                  base::read_text_file_as_string(
                      shaders / "post.vert" ) );
    UNWRAP_CHECK( postprocessing_fragment_shader_source,
                  base::read_text_file_as_string(
                      shaders / "post.frag" ) );
    UNWRAP_CHECK( postprocessing_vert_shader,
                  gl::Shader::create(
                      gl::e_shader_type::vertex,
                      postprocessing_vertex_shader_source ) );
    UNWRAP_CHECK( postprocessing_frag_shader,
                  gl::Shader::create(
                      gl::e_shader_type::fragment,
                      postprocessing_fragment_shader_source ) );

    RenderBufferMap buffers;
    for( e_render_buffer const buffer :
         refl::enum_values<e_render_buffer> ) {
      auto vertices    = make_unique<vector<GenericVertex>>();
      auto* p_vertices = vertices.get();
      using ERB        = e_render_buffer;
      bool const track_dirty =
          ( buffer == ERB::landscape ) ||
          ( buffer == ERB::landscape_annex ) ||
          ( buffer == ERB::landscape_anim_replace ) ||
          ( buffer == ERB::landscape_anim_enpixelate ) ||
          ( buffer == ERB::obfuscation ) ||
          ( buffer == ERB::obfuscation_annex );
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
          buffers[e_render_buffer{}]->vertex_array.bind();
      UNWRAP_CHECK( pgrm, NormalProgramType::create(
                              vert_shader, frag_shader ) );
      return std::move( pgrm );
    }();

    pgrm["u_atlas"_t] = 0; // GL_TEXTURE0
    pgrm["u_noise"_t] = 1; // GL_TEXTURE1

    auto postprocessing_pgrm = [&] {
      // Some OpenGL drivers, during shader program validation,
      // seem to require a vertex array to be bound to include in
      // the validation process.
      auto va_binder =
          buffers[e_render_buffer{}]->vertex_array.bind();
      UNWRAP_CHECK( pgrm, PostProgramType::create(
                              postprocessing_vert_shader,
                              postprocessing_frag_shader ) );
      return std::move( pgrm );
    }();

    postprocessing_pgrm["u_source"_t] = 0; // GL_TEXTURE0

    // Color cycling. These are just no-op settings to make sure
    // that they have well-defined values, and also just to
    // trigger them once for the unit tests. But, in reality,
    // they will be set separately later.
    set_color_cycle_plans( pgrm, vector<pixel>{} );
    set_color_cycle_keys( pgrm, vector<pixel>{} );

    gfx::size const logical_screen_size =
        config.logical_screen_size;
    auto const u_screen_size =
        gl::vec2::from_size( logical_screen_size );
    pgrm["u_screen_size"_t]                = u_screen_size;
    postprocessing_pgrm["u_screen_size"_t] = u_screen_size;

    AtlasBuilder atlas_builder;
    AtlasLoadOutput atlas_output;

    for( SpriteSheetConfig const& sheet :
         config.sprite_sheets ) {
      CHECK_HAS_VALUE( load_sprite_sheet( atlas_builder, sheet,
                                          atlas_output ) );
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
    unordered_map<string_view, int> atlas_ids_fast;
    unordered_map<string_view, AsciiFont*> ascii_fonts_fast;
    for( auto const& [name, id] : atlas_output.atlas_ids )
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

    unordered_map<string, gfx::rect> atlas_trimmed_rects;
    for( auto const& [name, id] : atlas_output.atlas_ids )
      atlas_trimmed_rects[name] =
          atlas.dict.trimmed_bounds( id );
    CHECK( atlas_trimmed_rects.size() == atlas_ids_fast.size() );

    if( config.dump_atlas_png.has_value() ) {
      lg.info( "writing atlas png to {}.",
               *config.dump_atlas_png );
      CHECK_HAS_VALUE(
          stb::save_image( *config.dump_atlas_png, atlas.img ) );
    }

    size atlas_size = atlas.img.size_pixels();
    gl::Texture atlas_tx( std::move( atlas.img ) );
    pgrm["u_atlas_size"_t] = gl::vec2::from_size( atlas_size );

    size const noise_size = { .w = 640, .h = 640 };
    gfx::image noise_img  = create_noise_image( noise_size );
    // Do this before img gets moved from.
    if( config.dump_noise_png.has_value() ) {
      lg.info( "writing noise png to {}.",
               *config.dump_noise_png );
      CHECK_HAS_VALUE(
          stb::save_image( *config.dump_noise_png, noise_img ) );
    }
    gl::Texture noise_tx( std::move( noise_img ) );
    pgrm["u_noise_size"_t] = gl::vec2::from_size( noise_size );

    // Note some fields are not explicitly initialized here
    // (there are initialized in the constructor above).
    return new Impl(
        /*present_fn=*/std::move( present_fn ),
        /*program=*/std::move( pgrm ),
        /*postprocessing_program=*/
        std::move( postprocessing_pgrm ),
        /*buffers=*/std::move( buffers ),
        /*atlas_map=*/std::move( atlas.dict ),
        /*atlas_size=*/atlas_size,
        /*atlas_tx=*/std::move( atlas_tx ),
        /*noise_tx=*/std::move( noise_tx ),
        /*atlas_ids=*/std::move( atlas_output.atlas_ids ),
        /*atlas_ids_fast=*/std::move( atlas_ids_fast ),
        /*atlas_trimmed_rects=*/std::move( atlas_trimmed_rects ),
        /*atlas_burrow_ids=*/
        std::move( atlas_output.atlas_burrow_ids ),
        /*ascii_fonts=*/std::move( ascii_fonts ),
        /*ascii_fonts_fast=*/std::move( ascii_fonts_fast ),
        /*logical_screen_size=*/logical_screen_size,
        /*framebuffer_mode=*/config.framebuffer_mode );
  }

  void begin_pass_impl( e_render_buffer_phase const phase,
                        RenderPassOpts const& opts ) {
    if( !opts.clear_buffers ) return;
    // This should not affect the capacity.
    for( auto& [buffer, data] : buffers ) {
      if( buffer_phase( buffer ) != phase ) continue;
      // This will prevent resetting the buffers that don't get
      // redrawn each frame.
      if( data->track_dirty ) continue;
      auto& vertices = *buffers[buffer]->vertices;
      auto& emitter  = buffers[buffer]->emitter;
      [[maybe_unused]] size_t capacity_before_clear =
          vertices.capacity();
      vertices.clear();
      CHECK( vertices.capacity() == capacity_before_clear );
      CHECK( vertices.empty() );
      emitter.set_position( 0 );
    }
  }

  void begin_pass_normal( RenderPassOpts const& opts ) {
    return begin_pass_impl( e_render_buffer_phase::normal,
                            opts );
  }

  void begin_pass_postprocessing( RenderPassOpts const& opts ) {
    return begin_pass_impl(
        e_render_buffer_phase::postprocessing, opts );
  }

  int end_pass_normal() {
    int vertex_count = 0;
    for( auto& [buffer, data] : buffers ) {
      if( buffer_phase( buffer ) !=
          e_render_buffer_phase::normal )
        continue;
      render_buffer_normal( buffer );
      auto& vertices = *buffers[buffer]->vertices;
      vertex_count += vertices.size();
    }
    return vertex_count;
  }

  int end_pass_postprocessing() {
    int vertex_count = 0;
    for( auto& [buffer, data] : buffers ) {
      if( buffer_phase( buffer ) !=
          e_render_buffer_phase::postprocessing )
        continue;
      render_buffer_postprocessing( buffer );
      auto& vertices = *buffers[buffer]->vertices;
      vertex_count += vertices.size();
    }
    return vertex_count;
  }

  Emitter& curr_emitter() {
    return buffers[mods().buffer_mods.buffer]->emitter;
  }

  Painter painter() {
    return Painter( atlas_map, curr_emitter(),
                    mod_stack.top().painter_mods );
  }

  Typer typer( string_view const font_name,
               TextLayout const& layout, point const start,
               pixel const color, Painter const& painter ) {
    UNWRAP_CHECK( p_ascii_font,
                  base::lookup( ascii_fonts_fast, font_name ) );
    return Typer( painter, *p_ascii_font, layout, start, color );
  }

  Typer typer( string_view const font_name,
               TextLayout const& layout, point const start,
               pixel const color ) {
    UNWRAP_CHECK( p_ascii_font,
                  base::lookup( ascii_fonts_fast, font_name ) );
    return Typer( painter(), *p_ascii_font, layout, start,
                  color );
  }

  Typer typer( string_view const font_name,
               TextLayout const& layout ) {
    UNWRAP_CHECK( p_ascii_font,
                  base::lookup( ascii_fonts_fast, font_name ) );
    return Typer( painter(), *p_ascii_font, layout );
  }

  void clear_screen( gfx::pixel color ) { gl::clear( color ); }

  void set_logical_screen_size( size new_size ) {
    normal_program["u_screen_size"_t] =
        gl::vec2::from_size( new_size );
    postprocessing_program["u_screen_size"_t] =
        gl::vec2::from_size( new_size );
    logical_screen_size = new_size;
    recreate_postprocessing_framebuffer();
  }

  void set_viewport_no_cache( rect const viewport ) {
    gl::set_viewport( viewport );
  }

  void set_viewport( rect const viewport ) {
    physical_viewport_ = viewport;
    set_viewport_no_cache( viewport );
  }

  unordered_map<string_view, int> const& atlas_ids_fn() const {
    return atlas_ids_fast;
  }

  unordered_map<string, gfx::rect> const&
  atlas_trimmed_rects_fn() const {
    return atlas_trimmed_rects;
  }

  unordered_map<int, int> const& atlas_burrow_ids_fn() const {
    return atlas_burrow_ids;
  }

  RendererMods const& mods() const {
    DCHECK( !mod_stack.empty() );
    return mod_stack.top();
  }

  void mods_push_back( RendererMods&& mods ) {
    mod_stack.push( std::move( mods ) );
    e_render_buffer const buffer = mods.buffer_mods.buffer;
    if( buffers[buffer]->track_dirty )
      buffers[buffer]->dirty = true;
  }

  void mods_pop() {
    DCHECK( mod_stack.size() > 1 );
    mod_stack.pop();
  }

  void clear_buffer( e_render_buffer buffer ) {
    // This won't cause the data to be removed from the GPU, but
    // the effect will be the same, because when we run the
    // shader program we specify the number of vertices to run it
    // on, which will be zero after the following.
    buffers[buffer]->vertices->clear();
    buffers[buffer]->emitter.set_position( 0 );
  }

  long buffer_vertex_cur_pos(
      base::maybe<e_render_buffer> buffer = base::nothing ) {
    return get_emitter(
               buffer.value_or( mods().buffer_mods.buffer ) )
        .position();
  }

  VertexRange range_for( function_ref<void()> const f ) {
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

  static void set_color_cycle_plans(
      NormalProgramType& pgrm, vector<pixel> const& plans ) {
    vector<gl::ivec4> gl_plans;
    gl_plans.resize( plans.size() );
    transform( plans.begin(), plans.end(), gl_plans.begin(),
               gl::ivec4::from_pixel );
    CHECK( gl_plans.size() == plans.size() ); // sanity check.
    pgrm["u_color_cycle_targets"_t] = gl_plans;
  }

  static void set_color_cycle_keys(
      NormalProgramType& pgrm, span<pixel const> const plans ) {
    vector<gl::ivec3> gl_plans;
    gl_plans.resize( plans.size() );
    transform( plans.begin(), plans.end(), gl_plans.begin(),
               gl::ivec3::from_pixel );
    CHECK( gl_plans.size() == plans.size() ); // sanity check.
    pgrm["u_color_cycle_keys"_t] = gl_plans;
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
      span const segment{ start_iter, end_iter };
      VertexArray_t const& vertex_array =
          get_vertex_array( rng.buffer );
      vertex_array.buffer<0>().upload_data_modify( segment,
                                                   rng.start );
    }
  }

  void render_buffer_impl( auto& pgrm,
                           e_render_buffer const buffer ) {
    auto const& vertex_array = buffers[buffer]->vertex_array;
    auto& vertices           = *buffers[buffer]->vertices;
    bool& dirty              = buffers[buffer]->dirty;
    if( !buffers[buffer]->track_dirty || dirty )
      vertex_array.buffer<0>().upload_data_replace(
          vertices, gl::e_draw_mode::stat1c );
    dirty = false;
    // Still need to run even if it wasn't dirty because uniforms
    // may have changed.
    pgrm.run( vertex_array, vertices.size() );
  }

  void render_buffer_normal( e_render_buffer const buffer ) {
    render_buffer_impl( normal_program, buffer );
  }

  void render_buffer_postprocessing(
      e_render_buffer const buffer ) {
    render_buffer_impl( postprocessing_program, buffer );
  }

  void render_pass_direct_to_screen(
      function_ref<void()> const drawer,
      RenderPassOpts const& opts ) {
    begin_pass_normal( opts );
    drawer();
    end_pass_normal();
  }

  void render_pass_postprocessing_with_logical_resolution(
      function_ref<void()> const drawer,
      RenderPassOpts const& opts ) {
    {
      auto const _ = render_framebuffer.bind();
      begin_pass_normal( opts );
      drawer();
      rect const logical_screen_rect = {
        .origin = {}, .size = logical_screen_size };
      set_viewport_no_cache( logical_screen_rect );
      SCOPE_EXIT {
        set_viewport_no_cache( physical_viewport_ );
      };
      end_pass_normal();
    }
    {
      auto const _ = postprocessing_render_target_tx.bind();
      begin_pass_postprocessing( RenderPassOpts{} );
      gl::clear();
      {
        mod_stack.push( mod_stack.top() );
        SCOPE_EXIT { mod_stack.pop(); };
        mod_stack.top().buffer_mods.buffer =
            e_render_buffer::postprocessing;
        Painter painter = this->painter();
        painter.draw_solid_rect(
            { .origin = {}, .size = physical_viewport_.size },
            pixel::black() );
      }
      set_viewport_no_cache( physical_viewport_ );
      end_pass_postprocessing();
    }
  }

  void render_pass( function_ref<void()> const drawer,
                    RenderPassOpts const& opts ) {
    switch( framebuffer_mode_ ) {
      case e_render_framebuffer_mode::direct_to_screen: {
        render_pass_direct_to_screen( drawer, opts );
        break;
      }
      case e_render_framebuffer_mode::
          offscreen_with_logical_resolution: {
        render_pass_postprocessing_with_logical_resolution(
            drawer, opts );
        break;
      }
    }
    present_fn();
  }

  // Const members.
  PresentFn const present_fn;
  AtlasMap const atlas_map;
  size const atlas_size;
  gl::Texture const atlas_tx;
  TextureBinder const atlas_tx_binder;
  gl::Texture const noise_tx;
  unordered_map<string, int> const atlas_ids;
  unordered_map<string_view, int> const atlas_ids_fast;
  unordered_map<string, gfx::rect> const atlas_trimmed_rects;
  unordered_map<int, int> const atlas_burrow_ids;
  unordered_map<string, AsciiFont> const ascii_fonts;
  unordered_map<string_view, AsciiFont*> const ascii_fonts_fast;

  // TODO: we probably don't need to keep the mods in a std::s-
  // tack, instead we can keep them in the popper object since
  // those will be stored on the stack and popped in reverse
  // order as they were applied, which should do the same job.
  stack<RendererMods> mod_stack;
  rect physical_viewport_ = {};
  NormalProgramType normal_program;
  PostProgramType postprocessing_program;
  RenderBufferMap buffers;
  gfx::size logical_screen_size;
  gl::Texture postprocessing_render_target_tx;
  gl::Framebuffer render_framebuffer;
  e_render_framebuffer_mode framebuffer_mode_ = {};
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

Painter Renderer::painter() { return impl_->painter(); }

Typer Renderer::typer( TextLayout const& layout ) {
  return impl_->typer( kDefaultFontName, layout );
}

Typer Renderer::typer( TextLayout const& layout, point start,
                       pixel color ) {
  return impl_->typer( kDefaultFontName, layout, start, color );
}

Typer Renderer::typer( string_view font_name,
                       TextLayout const& layout, point start,
                       pixel color ) {
  return impl_->typer( font_name, layout, start, color );
}

Typer Renderer::typer( string_view font_name,
                       TextLayout const& layout, point start,
                       pixel color, Painter const& painter ) {
  return impl_->typer( font_name, layout, start, color,
                       painter );
}

Typer Renderer::typer( point start, pixel color ) {
  return impl_->typer( kDefaultFontName, TextLayout{}, start,
                       color );
}

Typer Renderer::typer() {
  return impl_->typer( kDefaultFontName, TextLayout{} );
}

AtlasMap const& Renderer::atlas() const {
  return impl_->atlas_map;
}

AsciiFont const& Renderer::ascii_font(
    std::string_view font_name ) const {
  UNWRAP_CHECK_T(
      AsciiFont const* p_ascii_font,
      base::lookup( impl_->ascii_fonts_fast, font_name ) );
  return *p_ascii_font;
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

gfx::e_resolution Renderer::named_logical_resolution() const {
  UNWRAP_CHECK(
      res, resolution_from_size( impl_->logical_screen_size ) );
  return res;
}

void Renderer::set_viewport( gfx::rect const viewport ) {
  impl_->set_viewport( viewport );
}

unordered_map<string_view, int> const& Renderer::atlas_ids()
    const {
  return impl_->atlas_ids_fn();
}

unordered_map<string, gfx::rect> const&
Renderer::atlas_trimmed_rects() const {
  return impl_->atlas_trimmed_rects_fn();
}

unordered_map<int, int> const& Renderer::atlas_burrow_ids()
    const {
  return impl_->atlas_burrow_ids_fn();
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
    function_ref<void( Renderer& ) const> drawer,
    RenderPassOpts const& opts ) {
  impl_->render_pass( [&] { drawer( *this ); }, opts );
}

e_render_framebuffer_mode Renderer::render_framebuffer_mode()
    const {
  return impl_->framebuffer_mode_;
}

void Renderer::set_render_framebuffer_mode(
    e_render_framebuffer_mode const mode ) {
  impl_->framebuffer_mode_ = mode;
}

void Renderer::set_color_cycle_stage( int stage ) {
  impl_->normal_program["u_color_cycle_stage"_t] = stage;
}

void Renderer::set_color_cycle_plans(
    vector<pixel> const& plans ) {
  impl_->set_color_cycle_plans( impl_->normal_program, plans );
}

void Renderer::set_color_cycle_keys(
    span<pixel const> const plans ) {
  impl_->set_color_cycle_keys( impl_->normal_program, plans );
}

void Renderer::set_uniform_depixelation_stage( double stage ) {
  impl_->normal_program["u_depixelation_stage"_t] = stage;
}

void Renderer::set_camera( gfx::dsize translation,
                           double zoom ) {
  impl_->normal_program["u_camera_translation"_t] =
      gl::vec2::from_dsize( translation );
  impl_->normal_program["u_camera_zoom"_t] = zoom;
}

void Renderer::clear_buffer( e_render_buffer buffer ) {
  impl_->clear_buffer( buffer );
}

void Renderer::testing_only_render_buffer(
    e_render_buffer buffer ) {
  impl_->render_buffer_normal( buffer );
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

VertexRange Renderer::range_for( function_ref<void()> f ) const {
  return impl_->range_for( f );
}

} // namespace rr
