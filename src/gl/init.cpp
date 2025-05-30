/****************************************************************
**init.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-12.
*
* Description: Initializes OpenGL given a window/context.
*
*****************************************************************/
#include "init.hpp"

// gl
#include "error.hpp"
#include "iface-glad.hpp"
#include "iface-logger.hpp"
#include "misc.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace gl {

namespace {

pair<unique_ptr<IOpenGL>, unique_ptr<OpenGLWithLogger>>
create_and_set_global_instance( bool enable_logger ) {
  unique_ptr<IOpenGL> iface = make_unique<gl::OpenGLGlad>();
  set_global_gl_implementation( iface.get() );
  unique_ptr<OpenGLWithLogger> logger;
  if( enable_logger ) {
    logger = make_unique<gl::OpenGLWithLogger>( iface.get() );
    // Keep it off by default.
    logger->enable_logging( false );
    set_global_gl_implementation( logger.get() );
  }
  return { std::move( iface ), std::move( logger ) };
}

string get_str( int what ) {
  char const* s = (char const*)GL_CHECK( glGetString( what ) );
  return s;
}

} // namespace

/****************************************************************
** DriverInfo
*****************************************************************/
string DriverInfo::pretty_print() const {
  string res;
  res += fmt::format( "OpenGL loaded:\n" );
  res += fmt::format( "  * Vendor:      {}.\n", vendor );
  res += fmt::format( "  * Renderer:    {}.\n", renderer );
  res += fmt::format( "  * Version:     {}.\n", version );
  res += fmt::format( "  * Max Tx Size: {}.", max_texture_size );
  // !! If adding another line here, add a new line to the end of
  // the previous one.
  return res;
}

/****************************************************************
** Public API
*****************************************************************/
InitResult init_opengl( InitOptions opts ) {
  // Doing this any earlier in the process doesn't seem to work.
  CHECK( gladLoadGL(), "Failed to initialize GLAD." );

  auto [iface, logger] = create_and_set_global_instance(
      opts.include_glfunc_logging );

  int max_texture_size = 0;
  GL_CHECK(
      glGetIntegerv( GL_MAX_TEXTURE_SIZE, &max_texture_size ) );

  DriverInfo driver_info{
    .vendor           = get_str( GL_VENDOR ),
    .version          = get_str( GL_VERSION ),
    .renderer         = get_str( GL_RENDERER ),
    .max_texture_size = gfx::size{ .w = max_texture_size,
                                   .h = max_texture_size } };

  // I guess we don't need depth testing...
  // GL_CHECK( glEnable( GL_DEPTH_TEST ) );
  // GL_CHECK( glDepthFunc( GL_LEQUAL ) );

  // Without this, alpha blending won't happen.
  GL_CHECK(
      glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );
  GL_CHECK( glEnable( GL_BLEND ) );

  // Texture 0 should be the default, but let's just set it
  // anyway to be sure.
  set_active_texture( e_gl_texture::tx_0 );

  set_viewport( opts.initial_window_physical_pixel_size );

  // Clear screen to black.
  clear( gfx::pixel::black() );

  return InitResult{
    .driver_info   = std::move( driver_info ),
    .iface         = std::move( iface ),
    .logging_iface = std::move( logger ),
  };
}

} // namespace gl
