/****************************************************************
**window.hpp
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

namespace rn::gui {

class Object {
public:
  Object()          = default;
  virtual ~Object() = default;

  Object( Object const& ) = default;
  Object( Object&& )      = default;

  Object& operator=( Object const& ) = default;
  Object& operator=( Object&& ) = default;

  ND virtual bool needs_redraw() const                       = 0;
  virtual void  draw( Texture const& tx, Coord coord ) const = 0;
  virtual Delta size() const                                 = 0;
  ND virtual bool accept_input( SDL_Event /*unused*/ ) {
    return false;
  }
};

class View : public Object {};

class SolidRectView : public View {
public:
  SolidRectView() = default;
  SolidRectView( Color color, Delta delta )
    : color_( color ), delta_( std::move( delta ) ) {}

  // Implement Object
  bool needs_redraw() const override;
  // Implement Object
  void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  Delta size() const override;

protected:
  Color color_;
  Delta delta_;
};

struct ViewDesc {
  Rect bounds() const {
    return Rect::from( coord, coord + view->size() );
  };
  Coord                 coord;
  std::unique_ptr<View> view;
};

class CompositeView : public View {
public:
  // Pass views by value.
  explicit CompositeView( std::vector<ViewDesc> views )
    : views_( std::move( views ) ) {}

  // Implement Object
  bool needs_redraw() const override;
  // Implement Object
  void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  Delta size() const override;

protected:
  std::vector<ViewDesc> views_;
};

class OneLineStringView : public View {
public:
  OneLineStringView( std::string msg, W size );

  // Implement Object
  bool needs_redraw() const override;
  // Implement Object
  void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  Delta size() const override;

protected:
  std::string   msg_;
  SolidRectView background_;
  Texture       tx;
};

enum class e_window_state { running, closed };

using ViewPtr = std::unique_ptr<View>;

class Window : public Object {
public:
  Window( std::string title, ViewPtr view )
    : window_state_( e_window_state::running ),
      title_( std::move( title ) ),
      view_( std::move( view ) ) {}

  e_window_state state() const { return window_state_; }

  // Implement Object
  bool needs_redraw() const override;
  // Implement Object
  void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  Delta size() const override;

  std::string const& title() const { return title_; }

protected:
  e_window_state        window_state_;
  std::string           title_;
  std::unique_ptr<View> view_;
};

using RenderFunc = std::function<void( void )>;
using WinPtr     = std::unique_ptr<Window>;

class WindowManager {
public:
  WindowManager( WinPtr window, Coord position );

  void draw_layout( Texture const& tx ) const;

  void run( RenderFunc const& render_fn );

  ND bool accept_input( SDL_Event );

private:
  Delta window_size() const;

  // Currently the WM can only have one window.
  std::unique_ptr<Window> window_;
  OneLineStringView       title_bar_;
  Coord                   position_;
};

void test_window();

void message_box( std::string_view  msg,
                  RenderFunc const& render_bg );

} // namespace rn::gui
