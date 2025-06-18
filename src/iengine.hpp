/****************************************************************
**iengine.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-27.
*
* Description: Interface for the controller of the main game
*              engine components.
*
*****************************************************************/
#pragma once

/****************************************************************
** Forward Decls
*****************************************************************/
namespace vid {
struct IVideo;
struct WindowHandle;
}

namespace sfx {
struct ISfx;
}

namespace rr {
struct Renderer;
struct IRendererSettings;
struct ITextometer;
}

namespace gfx {
struct Resolutions;
}

namespace rn {

/****************************************************************
** IEngine
*****************************************************************/
struct IEngine {
  virtual ~IEngine() = default;

  virtual vid::IVideo& video() = 0;

  virtual sfx::ISfx& sfx() = 0;

  virtual vid::WindowHandle const& window() = 0;

  virtual rr::Renderer& renderer_use_only_when_needed() = 0;

  // This one, unlike the direct general access to the renderer
  // from the method above, is safe to use freely from anywhere.
  virtual rr::IRendererSettings& renderer_settings() = 0;

  virtual gfx::Resolutions& resolutions() = 0;

  virtual rr::ITextometer& textometer() = 0;

  // Will pause the entire engine and block until the pause key
  // is pressed. In the meantime, the window title bar will say
  // "PAUSED" and a message will be printed on screen so that the
  // user knows the game is paused.
  virtual void pause() = 0;
};

} // namespace rn
