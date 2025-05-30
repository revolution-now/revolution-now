/****************************************************************
**framebuffer.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-27.
*
* Description: C++ representation of an OpenGL Framebuffer.
*
*****************************************************************/
#include "framebuffer.hpp"

// gl
#include "error.hpp"
#include "iface.hpp"
#include "texture.hpp"

// base
#include "base/logger.hpp"

using namespace std;

namespace gl {

using ::base::lg;

/****************************************************************
** Framebuffer
*****************************************************************/
Framebuffer::Framebuffer() {
  ObjId fb_id = 0;
  GL_CHECK( CALL_GL( gl_GenFramebuffers, 1, &fb_id ) );
  *this = Framebuffer( fb_id );
}

Framebuffer::Framebuffer( ObjId const fb_id )
  : base::zero<Framebuffer, ObjId>( fb_id ),
    bindable<Framebuffer>() {}

void Framebuffer::free_resource() {
  ObjId const fb_id = resource();
  CHECK( fb_id != 0 );
  GL_CHECK( CALL_GL( gl_DeleteFramebuffers, 1, &fb_id ) );
}

ObjId Framebuffer::current_bound() {
  GLint id = {};
  GL_CHECK(
      CALL_GL( gl_GetIntegerv, GL_FRAMEBUFFER_BINDING, &id ) );
  return (ObjId)id;
}

void Framebuffer::bind_obj_id( ObjId const new_id ) {
  GL_CHECK(
      CALL_GL( gl_BindFramebuffer, GL_FRAMEBUFFER, new_id ) );
}

void Framebuffer::set_color_attachment( Texture const& tx ) {
  auto const binder = bind();
  GL_CHECK( CALL_GL( gl_FramebufferTexture2D, GL_FRAMEBUFFER,
                     GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                     tx.resource(), 0 ) );
}

bool Framebuffer::is_framebuffer_complete() const {
  auto const binder = bind();

  auto const status = GL_CHECK(
      CALL_GL( gl_CheckFramebufferStatus, GL_FRAMEBUFFER ) );

  if( status != GL_FRAMEBUFFER_COMPLETE )
    lg.critical(
        "OpenGL framebuffer status check failed: status={}",
        status );

  return ( status == GL_FRAMEBUFFER_COMPLETE );
}

} // namespace gl
