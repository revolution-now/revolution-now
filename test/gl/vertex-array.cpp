/****************************************************************
**vertex-array.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-28.
*
* Description: Unit tests for the src/gl/vertex-array.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/gl/vertex-array.hpp"

// gl
#include "src/gl/iface-mock.hpp"

// refl
#include "refl/ext.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gl {
namespace {

using namespace std;
using namespace ::mock::matchers;

struct Vertex {
  vec3    v;
  float   y;
  int32_t i;
};

} // namespace
} // namespace gl

namespace refl {

// Reflection info for struct Scale.
template<>
struct traits<gl::Vertex> {
  using type = gl::Vertex;

  static constexpr type_kind kind      = type_kind::struct_kind;
  static constexpr std::string_view ns = "gl";
  static constexpr std::string_view name = "Vertex";

  using template_types = std::tuple<>;

  static constexpr std::tuple fields{
      refl::StructField{ "v", &gl::Vertex::v,
                         offsetof( type, v ) },
      refl::StructField{ "y", &gl::Vertex::y,
                         offsetof( type, y ) },
      refl::StructField{ "i", &gl::Vertex::i,
                         offsetof( type, i ) },
  };
};

} // namespace refl

namespace gl {
namespace {

TEST_CASE( "[vertex-array] creation" ) {
  gl::MockOpenGL mock;

  EXPECT_CALL( mock, gl_GetError() )
      .times( 23 )
      .returns( GL_NO_ERROR );

  // Construct VertexArrayNonTyped.
  EXPECT_CALL( mock, gl_GenVertexArrays( 1, Not( Null() ) ) )
      .sets_arg<1>( 21 );

  // Construct vertex buffer.
  EXPECT_CALL( mock, gl_GenBuffers( 1, Not( Null() ) ) )
      .sets_arg<1>( 41 );

  // Bind vertex array.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 20 );
  EXPECT_CALL( mock, gl_BindVertexArray( 21 ) );

  // Bind vertex buffer.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 40 );
  EXPECT_CALL( mock, gl_BindBuffer( GL_ARRAY_BUFFER, 41 ) );

  // Call to get max allowed attributes.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_MAX_VERTEX_ATTRIBS,
                                     Not( Null() ) ) )
      .sets_arg<1>( 10 )
      .times( 3 );

  // Register attribute 0 (vec3).
  EXPECT_CALL( mock,
               gl_VertexAttribPointer(
                   /*index=*/0, /*size=*/3, /*type=*/GL_FLOAT,
                   /*normalized=*/false, /*stride=*/
                   sizeof( Vertex ),
                   /*pointer=*/nullptr ) );
  EXPECT_CALL( mock, gl_EnableVertexAttribArray( 0 ) );

  // Register attribute 1 (float).
  EXPECT_CALL(
      mock,
      gl_VertexAttribPointer(
          /*index=*/1, /*size=*/1, /*type=*/GL_FLOAT,
          /*normalized=*/false, /*stride=*/sizeof( Vertex ),
          /*pointer=*/(void*)offsetof( Vertex, y ) ) );
  EXPECT_CALL( mock, gl_EnableVertexAttribArray( 1 ) );

  // Register attribute 2 (int32_t).
  EXPECT_CALL( mock,
               gl_VertexAttribIPointer(
                   /*index=*/2, /*size=*/1, /*type=*/GL_INT,
                   /*stride=*/sizeof( Vertex ),
                   /*pointer=*/(void*)offsetof( Vertex, i ) ) );
  EXPECT_CALL( mock, gl_EnableVertexAttribArray( 2 ) );

  // Unbind vertex buffer.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 41 );
  EXPECT_CALL( mock, gl_BindBuffer( GL_ARRAY_BUFFER, 40 ) );
  EXPECT_CALL( mock, gl_GetIntegerv( GL_ARRAY_BUFFER_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 40 );

  // Unbind vertex array.
  EXPECT_CALL( mock, gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 21 );
  EXPECT_CALL( mock, gl_BindVertexArray( 20 ) );
  EXPECT_CALL( mock, gl_GetIntegerv( GL_VERTEX_ARRAY_BINDING,
                                     Not( Null() ) ) )
      .sets_arg<1>( 20 );

  // Delete vertex buffer.
  EXPECT_CALL( mock, gl_DeleteBuffers( 1, Pointee( 41 ) ) );

  // Delete vertex array.
  EXPECT_CALL( mock, gl_DeleteVertexArrays( 1, Pointee( 21 ) ) );

  // The call.
  VertexArray<VertexBuffer<Vertex>> arr;
}

} // namespace
} // namespace gl
