/****************************************************************
**texture.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-12.
*
* Description: C++ representation of an OpenGL texture.
*
*****************************************************************/
#include "texture.hpp"

// gl
#include "error.hpp"
#include "iface.hpp"

using namespace std;

namespace gl {

/****************************************************************
** Texture
*****************************************************************/
Texture::Texture() {
  ObjId tx_id = 0;
  GL_CHECK( CALL_GL( gl_GenTextures, 1, &tx_id ) );
  *this = Texture( tx_id );
}

Texture::Texture( ObjId tx_id )
  : base::zero<Texture, ObjId>( tx_id ), bindable<Texture>() {
  auto binder = bind();
  // Configure how OpenGL maps coordinate to texture pixel.
  GL_CHECK( CALL_GL( gl_TexParameteri, GL_TEXTURE_2D,
                     GL_TEXTURE_MIN_FILTER, GL_NEAREST ) );
  // Consider using GL_LINEAR here for magnifications to get a
  // more smoothed look.
  GL_CHECK( CALL_GL( gl_TexParameteri, GL_TEXTURE_2D,
                     GL_TEXTURE_MAG_FILTER, GL_NEAREST ) );

  GL_CHECK( CALL_GL( gl_TexParameteri, GL_TEXTURE_2D,
                     GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
  GL_CHECK( CALL_GL( gl_TexParameteri, GL_TEXTURE_2D,
                     GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );
}

Texture::Texture( gfx::image const& img ) : Texture() {
  set_image( img );
}

void Texture::set_image( gfx::image const& img ) {
  auto binder = bind();
  GL_CHECK( CALL_GL( gl_TexImage2D, GL_TEXTURE_2D, 0, GL_RGBA,
                     img.width_pixels(), img.height_pixels(), 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, img.data() ) );
}

void Texture::free_resource() {
  ObjId tx_id = resource();
  DCHECK( tx_id != 0 );
  GL_CHECK( CALL_GL( gl_DeleteTextures, 1, &tx_id ) );
}

ObjId Texture::current_bound() {
  GLint id;
  GL_CHECK(
      CALL_GL( gl_GetIntegerv, GL_TEXTURE_BINDING_2D, &id ) );
  return (ObjId)id;
}

void Texture::bind_obj_id( ObjId new_id ) {
  GL_CHECK( CALL_GL( gl_BindTexture, GL_TEXTURE_2D, new_id ) );
}

} // namespace gl
