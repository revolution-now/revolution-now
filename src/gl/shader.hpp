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
            int                        num_vertices );

protected:
  ProgramNonTyped( ObjId id );

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
    // TODO: check attributes.
    return Program( std::move( pgrm_non_typed ) );
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
