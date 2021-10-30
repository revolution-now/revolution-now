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

struct Program;

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
    attacher( Program const& pgrm, Shader const& shader );

    ~attacher();

    NO_COPY_NO_MOVE( attacher );

  private:
    Program const& pgrm_;
    Shader const&  shader_;
  };

public:
  static base::expect<Shader, std::string> create(
      e_shader_type type, std::string const& code );

  attacher attach( Program const& pgrm ) const {
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
** Shader Program
*****************************************************************/
struct Program : base::zero<Program, ObjId> {
  static base::expect<Program, std::string> create(
      Shader const& vertex, Shader const& fragment );

  ObjId id() const { return resource(); }

  void use() const;

  void run( VertexArrayNonTyped const& vert_array,
            int                        num_vertices );

private:
  Program( ObjId id );

private:
  // Implement base::zero.
  friend base::zero<Program, ObjId>;

  void free_resource();
};

} // namespace gl
