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

pair<unique_ptr<IOpenGL>, OpenGLWithLogger*>
create_and_set_global_instance( bool enable_logger ) {
  unique_ptr<IOpenGL> iface;
  iface                    = make_unique<gl::OpenGLGlad>();
  OpenGLWithLogger* logger = nullptr;
  if( enable_logger ) {
    auto with_logging =
        make_unique<gl::OpenGLWithLogger>( iface.get() );
    logger = with_logging.get();
    with_logging->enable_logging( true );
    iface = make_unique<gl::OpenGLWithLogger>( iface.get() );
  }
  set_global_gl_implementation( iface.get() );
  return { std::move( iface ), logger };
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
  res += fmt::format( "OpenGL loaded:" );
  res += fmt::format( "  * Vendor:      {}.", vendor );
  res += fmt::format( "  * Renderer:    {}.", renderer );
  res += fmt::format( "  * Version:     {}.", version );
  res += fmt::format( "  * Max Tx Size: {}.", max_texture_size );
  return res;
}

/****************************************************************
** Public API
*****************************************************************/
InitResult init_opengl( InitOptions opts ) {
  // Doing this any earlier in the process doesn't seem to work.
  CHECK( gladLoadGL(), "Failed to initialize GLAD." );

  auto [iface, logger] = create_and_set_global_instance(
      opts.enable_glfunc_logging );

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

  set_viewport( opts.initial_window_physical_pixel_size );

  // Clear screen to black.
  clear( gfx::pixel::black() );

  return InitResult{
      .driver_info   = std::move( driver_info ),
      .iface         = std::move( iface ),
      .logging_iface = logger,
  };
}

} // namespace gl
