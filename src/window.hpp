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

// Revolution Now
#include "aliases.hpp"
#include "input.hpp"
#include "sdl-util.hpp"

// c++ standard library
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace rn::input {
struct event_t;
}

namespace rn::ui {

class Object {
public:
  Object()                = default;
  virtual ~Object()       = default;
  Object( Object const& ) = delete;
  Object( Object&& )      = delete;

  Object& operator=( Object const& ) = delete;
  Object& operator=( Object&& ) = delete;

  virtual void draw( Texture const& tx, Coord coord ) const = 0;
  // This is the physical size of the object in pixels.
  ND virtual Delta delta() const = 0;
  // Given a position, returns a bounding rect.  We need to be
  // given a position here because Objects don't know their
  // position, only their size.
  ND virtual Rect rect( Coord position ) const {
    return Rect::from( position, delta() );
  }
  ND virtual bool accept_input(
      input::event_t const& /*unused*/ ) {
    return false;
  }
};

/****************************************************************
** Views
*****************************************************************/
class View : public Object {};

// This is a View coupled with a coordinate representing the po-
// sition of the upper-left corner of the view. Note that the co-
// ordinate is in the coordinate system of the parent view or
// window (whose position in turn will not be know by this
// struct).
struct PositionedView {
  PositionedView( ObserverPtr<View> view_, Coord const& coord_ )
    : view( view_ ), coord( coord_ ) {}
  ObserverCPtr<View> const view;
  Coord const              coord;
};

// Same as above, but owns the view.  The
class OwningPositionedView : public PositionedView {
public:
  OwningPositionedView( std::unique_ptr<View> view,
                        Coord const&          coord )
    : PositionedView( ObserverPtr<View>( view.get() ), coord ),
      view_( std::move( view ) ) {}

  View const* get() const { return view_.get(); }
  View*       get() { return view_.get(); }

private:
  std::unique_ptr<View> view_;
};

class CompositeView : public View {
public:
  // Implement Object
  void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override;

  virtual int                   count() const       = 0;
  virtual PositionedView const& at( int idx ) const = 0;

  template<typename T, typename Child>
  struct IterT {
    T*    cview;
    int   idx;
    auto& operator*() { return cview->at( idx ); }
    void  operator++() { ++idx; }
    bool  operator!=( Child const& rhs ) {
      return rhs.idx != idx;
    }
  };
  struct iter : public IterT<CompositeView, iter> {};
  struct citer : public IterT<CompositeView const, citer> {};

  iter  begin() { return iter{{this, 0}}; }
  iter  end() { return iter{{this, count()}}; }
  citer begin() const { return citer{{this, 0}}; }
  citer end() const { return citer{{this, count()}}; }
};

class ViewVector : public CompositeView {
public:
  ViewVector( std::vector<OwningPositionedView> views )
    : views_( std::move( views ) ) {}

  // Implement CompositeView
  PositionedView const& at( int idx ) const override;
  // Implement CompositeView
  int count() const override { return int( views_.size() ); }

private:
  std::vector<OwningPositionedView> views_;
};

/****************************************************************
** Primitive Views
*****************************************************************/
class SolidRectView : public View {
public:
  SolidRectView( Color color, Delta delta )
    : color_( color ), delta_( delta ) {}

  // Implement Object
  void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override { return delta_; }

protected:
  Color color_;
  Delta delta_;
};

class OneLineStringView : public View {
public:
  OneLineStringView( std::string msg );

  // Implement Object
  void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override { return tx.size(); }

protected:
  Texture tx;
};

/****************************************************************
** WindowManager
*****************************************************************/
using RenderFunc = std::function<void( void )>;

enum class e_window_state { running, closed };
enum class e_wm_input_result { unhandled, handled, quit };

class WindowManager {
public:
  void draw_layout( Texture const& tx ) const;

  void run( RenderFunc const& render_fn );

  ND e_wm_input_result accept_input( SDL_Event const& event );

  void add_window( std::string title, std::unique_ptr<View> view,
                   Coord position );

private:
  struct window {
    window( std::string title_, std::unique_ptr<View> view_,
            Coord position_ )
      : window_state( e_window_state::running ),
        title( std::move( title_ ) ),
        view( std::move( view_ ) ),
        title_bar(),
        position( position_ ) {
      title_bar = std::make_unique<OneLineStringView>( title );
    }

    void  draw( Texture const& tx ) const;
    Delta delta() const;
    Rect  rect() const;
    Coord inside_border() const;

    e_window_state                     window_state;
    std::string                        title;
    std::unique_ptr<View>              view;
    std::unique_ptr<OneLineStringView> title_bar;
    Coord                              position;
  };

  // Gets the window with focus, throws if no windows.
  window& focused();

  std::vector<window> windows_;
};

/****************************************************************
** High-level Methods
*****************************************************************/
void test_window();

void message_box( std::string_view  msg,
                  RenderFunc const& render_bg );

} // namespace rn::ui
