/****************************************************************
**shader.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-30.
*
* Description: OpenGL Shader Wrappers.
*
*****************************************************************/
#pragma once

// gl
#include "types.hpp"
#include "uniform.hpp"
#include "vertex-array.hpp"

// base
#include "base/ct-string.hpp"
#include "base/expect.hpp"
#include "base/macros.hpp"
#include "base/meta.hpp"
#include "base/to-str.hpp"
#include "base/valid.hpp"
#include "base/zero.hpp"

// C++ standard library
#include <unordered_map>

namespace gl {

struct ProgramNonTyped;

/****************************************************************
** Shader Type
*****************************************************************/
enum class e_shader_type { vertex, fragment };

void to_str( e_shader_type type, std::string& out );

/****************************************************************
** Vertex/Fragment Shader
*****************************************************************/
struct Shader : base::zero<Shader, ObjId> {
 private:
  struct [[nodiscard]] attacher {
    attacher( ProgramNonTyped const& pgrm,
              Shader const&          shader );

    ~attacher();

    NO_COPY_NO_MOVE( attacher );

   private:
    ProgramNonTyped const& pgrm_;
    Shader const&          shader_;
  };

 public:
  static base::expect<Shader, std::string> create(
      e_shader_type type, std::string const& code );

  attacher attach( ProgramNonTyped const& pgrm ) const {
    return attacher( pgrm, *this );
  }

  ObjId id() const { return resource(); }

 private:
  Shader( ObjId id );

 private:
  // Implement base::zero.
  friend base::zero<Shader, ObjId>;

  void free_resource();
};

/****************************************************************
** ProgramNonTyped
*****************************************************************/
struct ProgramNonTyped : base::zero<ProgramNonTyped, ObjId> {
  static base::expect<ProgramNonTyped, std::string> create(
      Shader const& vertex, Shader const& fragment );

  ObjId id() const { return resource(); }

  void use() const;

  void run( VertexArrayNonTyped const& vert_array,
            int                        num_vertices ) const;

 protected:
  ProgramNonTyped( ObjId id );

  int num_input_attribs() const;

  // Gets the compound type of the attribute at idx. idx is ex-
  // pected to be < the value returned by num_input_attribs().
  // Note that this index is not necessarily the same as the lo-
  // cation as specified in the shader source code. Some drivers
  // appear to reorder them. So this function will also return
  // the location.
  base::expect<
      std::pair<e_attrib_compound_type, int /*location*/>,
      std::string>
  attrib_compound_type( int idx ) const;

 private:
  // Implement base::zero.
  friend base::zero<ProgramNonTyped, ObjId>;

  void free_resource();
};

/****************************************************************
** Program
*****************************************************************/
template<typename InputAttribTypeList, typename ProgramUniforms>
struct Program : ProgramNonTyped {
  static base::expect<Program, std::string> create(
      Shader const& vertex, Shader const& fragment ) {
    UNWRAP_RETURN( pgrm_non_typed,
                   ProgramNonTyped::create( vertex, fragment ) );
    auto pgrm = Program( std::move( pgrm_non_typed ) );
#if 1 // to enable input vertex attribute type checking.
    HAS_VALUE_OR_RET( validate_program( pgrm ) );
#endif
    HAS_VALUE_OR_RET( try_initialize_uniforms( pgrm ) );
    return pgrm;
  }

  template<typename... VertexBuffers>
  void run( VertexArray<VertexBuffers...> const& vert_array,
            int num_vertices ) requires
      std::is_same_v<InputAttribTypeList,
                     typename VertexArray<
                         VertexBuffers...>::AttribTypeList> {
    this->ProgramNonTyped::run( vert_array, num_vertices );
  }

  /* clang-format off */
private:
   /* clang-format on */
   static base::valid_or<std::string>
   try_initialize_uniforms( Program& pgrm ) {
    static constexpr int kNumUniforms =
        std::tuple_size_v<decltype( ProgramUniforms::uniforms )>;
    base::valid_or<std::string> res = base::valid;
    FOR_CONSTEXPR_IDX( Idx, kNumUniforms ) {
      res = std::get<Idx>( pgrm.uniforms_.values ).try_set( {} );
      if( !res.valid() ) {
        static constexpr std::string_view kUniformName =
            std::get<Idx>( ProgramUniforms::uniforms ).name;
        res = fmt::format( "failed to set uniform named {}: {}",
                           kUniformName, res.error() );
        return true; // stop iterating.
      }
      return false; // keep iterating.
    };
    return res;
  }

  static base::valid_or<std::string> validate_program(
      Program const& pgrm ) {
    static constexpr size_t kNumAttribs =
        mp::list_size_v<InputAttribTypeList>;
    if( pgrm.num_input_attribs() != kNumAttribs )
      return fmt::format(
          "shader program expected to have {} input attributes "
          "but has {}.",
          kNumAttribs, pgrm.num_input_attribs() );

    // We need to make one pass over the attributes to fill out
    // this map first because in some drivers the ordering of the
    // vertex attributes as queried by glGetActiveAttrib does not
    // correspond to the location as specified in the shader
    // source code (some drivers seem to alphabetize them).
    std::unordered_map<int, e_attrib_compound_type> idx_to_type;
    for( int i = 0; i < int( kNumAttribs ); ++i ) {
      UNWRAP_RETURN( info, pgrm.attrib_compound_type( i ) );
      auto [type, location] = info;
      idx_to_type[location] = type;
    };

    std::string err;
    // Now check that the types match their expected types.
    FOR_CONSTEXPR_IDX( Location, kNumAttribs ) {
      using AttribType = std::tuple_element_t<
          Location, mp::to_tuple_t<InputAttribTypeList>>;
      DCHECK( idx_to_type.contains( Location ) );
      e_attrib_compound_type expected =
          attrib_traits<AttribType>::compound_type;
      if( idx_to_type[Location] != expected ) {
        err = fmt::format(
            "shader program input attribute at index {} "
            "(zero-based) expected to have type {} but has type "
            "{}.",
            Location, to_GL_str( expected ),
            to_GL_str( idx_to_type[Location] ) );
        return true; // stop iteration.
      }
      return false; // keep iterating.
    };
    if( !err.empty() ) return err;
    return base::valid;
  }

  static constexpr size_t kInvalidUniformName = 1234567;

  template<size_t... Idx>
  constexpr static size_t find_uniform_index_impl(
      std::string_view what, std::index_sequence<Idx...> ) {
    size_t res      = kInvalidUniformName;
    auto&  uniforms = ProgramUniforms::uniforms;
    ( ( res = ( std::get<Idx>( uniforms ).name == what ) ? Idx
                                                         : res ),
      ... );
    return res;
  }

  constexpr static size_t find_uniform_index(
      std::string_view what ) {
    return find_uniform_index_impl(
        what, std::make_index_sequence<std::tuple_size_v<
                  decltype( ProgramUniforms::uniforms )>>() );
  }

  /* clang-format off */
private:
  Program( ProgramNonTyped pgrm )
    : ProgramNonTyped( std::move( pgrm ) ),
      uniforms_( id(), ProgramUniforms::uniforms ) {}
  /* clang-format on */

  using SpecTuple  = decltype( ProgramUniforms::uniforms );
  using SpecIdxSeq = decltype( std::make_index_sequence<
                               std::tuple_size_v<SpecTuple>>() );

  template<typename T, typename Sequence>
  struct UniformArray;

  template<typename... Specs, size_t... Idx>
  struct UniformArray<std::tuple<Specs...> const,
                      std::index_sequence<Idx...>> {
    UniformArray( ObjId                       pgrm_id,
                  std::tuple<Specs...> const& specs )
      : values{ { pgrm_id, std::get<Idx>( specs ).name }... } {}

    using values_t =
        std::tuple<Uniform<typename Specs::type>...>;
    values_t values;
  };

  using UniformTuple = UniformArray<SpecTuple, SpecIdxSeq>;
  UniformTuple uniforms_;

 public:
  // If a call to this function fails to compile it is either be-
  // cause the name of the uniform is wrong or the type of the
  // parameter does not match the type of the uniform. The Bar-
  // rier is to prevent the user from trying to set the N tem-
  // plate parameter.
  template<
      base::ct_string name, size_t... Barrier,
      size_t N = find_uniform_index( std::string_view( name ) )>
  void set_uniform(
      typename std::tuple_element_t<
          N, typename UniformTuple::values_t>::type const&
          val ) {
    static_assert(
        sizeof...( Barrier ) == 0,
        "Too many template parameters given to set_uniform." );
    std::get<N>( uniforms_.values ).set( val );
  }

 private:
  // Implement base::zero.
  friend base::zero<Program, ObjId>;

  void free_resource();
};

} // namespace gl
