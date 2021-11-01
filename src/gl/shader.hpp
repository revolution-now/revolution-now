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
#include "vertex-array.hpp"

// base
#include "base/expect.hpp"
#include "base/macros.hpp"
#include "base/meta.hpp"
#include "base/to-str.hpp"
#include "base/zero.hpp"

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
  // pected to be < the value returned by num_input_attribs(). It
  // will also make sure that the location index of the attribute
  // is the same as the index passed in, otherwise it will return
  // an error.
  base::expect<e_attrib_compound_type, std::string>
  attrib_compound_type( int idx ) const;

private:
  // Implement base::zero.
  friend base::zero<ProgramNonTyped, ObjId>;

  void free_resource();
};

/****************************************************************
** Program
*****************************************************************/
template<typename InputAttribTypeList>
struct Program : ProgramNonTyped {
  static base::expect<Program, std::string> create(
      Shader const& vertex, Shader const& fragment ) {
    UNWRAP_RETURN( pgrm_non_typed,
                   ProgramNonTyped::create( vertex, fragment ) );
    auto pgrm = Program( std::move( pgrm_non_typed ) );
#if 1 // to disable input vertex attribute type checking.
    static constexpr size_t kNumAttribs =
        mp::list_size_v<InputAttribTypeList>;
    if( pgrm.num_input_attribs() != kNumAttribs )
      return fmt::format(
          "shader program expected to have {} input attributes "
          "but has {}.",
          kNumAttribs, pgrm.num_input_attribs() );
    std::string err;
    FOR_CONSTEXPR_IDX( AttribIdx, kNumAttribs ) {
      using AttribType = std::tuple_element_t<
          AttribIdx, mp::to_tuple_t<InputAttribTypeList>>;
      base::expect<e_attrib_compound_type, std::string> type =
          pgrm.attrib_compound_type( AttribIdx );
      if( !type.has_value() ) {
        err = type.error();
        return true; // stop iteration.
      }
      e_attrib_compound_type expected =
          attrib_traits<AttribType>::compound_type;
      if( *type != expected ) {
        err = fmt::format(
            "shader program input attribute at index {} "
            "(zero-based) expected to have type {} but has type "
            "{}.",
            AttribIdx, to_GL_str( expected ),
            to_GL_str( *type ) );
        return true; // stop iteration.
      }
      return false; // keep iterating.
    };
    if( !err.empty() ) return err;
#endif
    return pgrm;
  }

  template<typename... VertexBuffers>
  void run( VertexArray<VertexBuffers...> const& vert_array,
            int num_vertices ) requires
      std::is_same_v<InputAttribTypeList,
                     typename VertexArray<
                         VertexBuffers...>::attrib_type_list> {
    this->ProgramNonTyped::run( vert_array, num_vertices );
  }

  /* clang-format off */
private:
  Program( ProgramNonTyped pgrm )
    : ProgramNonTyped( std::move( pgrm ) ) {}
  /* clang-format on */

private:
  // Implement base::zero.
  friend base::zero<Program, ObjId>;

  void free_resource();
};

} // namespace gl
