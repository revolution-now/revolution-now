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
#include <memory>
#include <string_view>
#include <vector>

namespace rn {
namespace gui {

enum class e_window_state {
  running,
  closed
};

class Object {
public:
  virtual ~Object() {}
  ND virtual bool needs_redraw() const = 0;
  virtual void draw( Texture const& tx, Coord coord ) const = 0;
  virtual Delta size() const = 0;
  ND virtual bool accept_input( SDL_Event ) { return false; }
};

class Window : public Object {
public:
  Window();
  virtual ~Window() {}
  e_window_state state() const { return window_state; }
protected:
  e_window_state window_state;
};

class View : public Object {
public:
  virtual ~View() {}
};

class SolidRectView : public View {
public:
  SolidRectView( Color const& color, Delta delta )
    : color_( color ), delta_( delta ) {}

  // Implement Object
  virtual bool needs_redraw() const override;
  // Implement Object
  virtual void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  virtual Delta size() const override;

protected:
  Color color_;
  Delta delta_;
};

struct ViewDesc {
  Rect bounds() const {
    return Rect::from( coord, coord+view->size() );
  };

  std::unique_ptr<View> view;
  Coord coord;
};

class CompositeView : public View {
public:
  // Pass views by value.
  CompositeView( std::vector<ViewDesc> views )
    : views_( std::move( views ) ) {}

  // Implement Object
  virtual bool needs_redraw() const override;
  // Implement Object
  virtual void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  virtual Delta size() const override;

protected:
  std::vector<ViewDesc> views_;
};

using RenderFunc = std::function<void(void)>;

void message_box( std::string_view msg, RenderFunc render_bg );

} // namespace gui
} // namespace rn
