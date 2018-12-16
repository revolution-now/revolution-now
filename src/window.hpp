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
  ObserverPtr<View> const view;
  Coord const             coord;
};
struct PositionedViewConst {
  ObserverCPtr<View> const view;
  Coord const              coord;
};

// Same as above, but owns the view.  The
class OwningPositionedView {
public:
  OwningPositionedView( std::unique_ptr<View> view,
                        Coord const&          coord )
    : view_( std::move( view ) ), coord_( coord ) {}

  ObserverCPtr<View> view() const {
    return ObserverCPtr<View>( view_.get() );
  }
  ObserverPtr<View> view() {
    return ObserverPtr<View>( view_.get() );
  }
  Coord const& coord() const { return coord_; }
  Coord&       coord() { return coord_; }

private:
  std::unique_ptr<View> view_;
  Coord                 coord_;
};

class CompositeView : public View {
public:
  // Implement Object
  void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override;

  bool accept_input( input::event_t const& event ) override;

  virtual int count() const = 0;

  virtual PositionedView      at( int idx )       = 0;
  virtual PositionedViewConst at( int idx ) const = 0;

  template<typename T, typename Child>
  struct IterT {
    T*   cview;
    int  idx;
    auto operator*() { return cview->at( idx ); }
    void operator++() { ++idx; }
    bool operator!=( Child const& rhs ) {
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
  ViewVector() {}

  ViewVector( std::vector<OwningPositionedView> views )
    : views_( std::move( views ) ) {}

  // Implement CompositeView
  PositionedView      at( int idx ) override;
  PositionedViewConst at( int idx ) const override;
  // Implement CompositeView
  int count() const override { return int( views_.size() ); }

  void push_back( OwningPositionedView view ) {
    views_.push_back( std::move( view ) );
  }

private:
  std::vector<OwningPositionedView> views_;
};

/****************************************************************
** Primitive Views
*****************************************************************/
class SolidRectView : public View {
public:
  SolidRectView( Color color ) : color_( color ), delta_{} {}

  SolidRectView( Color color, Delta delta )
    : color_( color ), delta_( delta ) {}

  // Implement Object
  void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override { return delta_; }

  void set_delta( Delta const& delta ) { delta_ = delta; }

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
** Derived Views
*****************************************************************/
enum class e_option_active { inactive, active };

class OptionSelectItemView : public CompositeView {
public:
  OptionSelectItemView( std::string msg )
    : active_{e_option_active::inactive},
      background_active_{Color::red()},
      background_inactive_{Color::black()},
      line_( msg ) {
    background_active_.set_delta( line_.delta() );
    background_inactive_.set_delta( line_.delta() );
  }

  // Implement CompositeView
  PositionedView      at( int idx ) override;
  PositionedViewConst at( int idx ) const override;
  // Implement CompositeView
  int count() const override { return 2; }

  void set_active( e_option_active active ) { active_ = active; }

private:
  e_option_active   active_;
  SolidRectView     background_active_;
  SolidRectView     background_inactive_;
  OneLineStringView line_;
};

class OptionSelectView : public ViewVector {
public:
  OptionSelectView( StrVec const& options,
                    int           initial_selection );

  bool accept_input( input::event_t const& event ) override;

private:
  ObserverPtr<OptionSelectItemView> get_view( int item );
  void                              set_selected( int item );

  int selected_;
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
