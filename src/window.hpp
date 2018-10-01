/****************************************************************
* window.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-30.
*
* Description: Handles windowing system for user interaction.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "sdl-util.hpp"

#include <functional>
#include <string_view>

namespace rn {
namespace gui {

enum class e_window_state {
  running,
  closed
};

class Object {
public:
  virtual ~Object() {}
  virtual bool needs_redraw() const = 0;
  virtual void draw( Texture tx, Rect clip ) const = 0;
  virtual Rect size() const = 0;
  virtual bool accept_input( SDL_Event event ) = 0;
};

class Window : public Object {
public:
  virtual ~Window() {}
  virtual e_window_state state() const = 0;
};

using RenderFunc = std::function<void(void)>;

void message_box( std::string_view msg, RenderFunc render_bg );

} // namespace gui
} // namespace rn
