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

class Object {
public:
  virtual ~Object() {}
  ND virtual bool needs_redraw() const = 0;
  virtual void draw( Texture const& tx, Coord coord ) const = 0;
  virtual Delta size() const = 0;
  ND virtual bool accept_input( SDL_Event ) { return false; }
};

class View : public Object {
public:
  virtual ~View() {}
};

class SolidRectView : public View {
public:
  SolidRectView() = default;
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
  ViewDesc( ViewDesc&& ) = default;
  Rect bounds() const {
    return Rect::from( coord, coord+view->size() );
  };
  Coord coord;
  std::unique_ptr<View> view;
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

class TitleBarView : public View {
public:
  TitleBarView( std::string title, W size );

  // Implement Object
  virtual bool needs_redraw() const override;
  // Implement Object
  virtual void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  virtual Delta size() const override;

protected:
  std::string title_;
  SolidRectView background_;
  Texture tx;
};

enum class e_window_state {
  running,
  closed
};

class Window : public Object {
public:
  Window( std::string const& title, std::unique_ptr<View> view )
    : window_state_( e_window_state::running ),
      title_bar_( title, view->size().w ),
      view_( std::move( view ) ) {}

  virtual ~Window() {}
  e_window_state state() const { return window_state_; }

  // Implement Object
  virtual bool needs_redraw() const override;
  // Implement Object
  virtual void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  virtual Delta size() const override;

protected:
  e_window_state window_state_;
  TitleBarView title_bar_;
  std::unique_ptr<View> view_;
};

void test_window();

using RenderFunc = std::function<void(void)>;

void message_box( std::string_view msg, RenderFunc render_bg );

} // namespace gui
} // namespace rn
