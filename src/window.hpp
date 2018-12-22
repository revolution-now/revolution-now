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
#include "errors.hpp"
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

  virtual PositionedViewConst at_const( int idx ) const = 0;
  virtual PositionedView      at( int idx );

  struct iter {
    CompositeView* cview;
    int            idx;
    auto           operator*() { return cview->at( idx ); }
    void           operator++() { ++idx; }
    bool operator!=( iter const& rhs ) { return rhs.idx != idx; }
  };
  struct citer {
    CompositeView const* cview;
    int                  idx;
    auto operator*() { return cview->at_const( idx ); }
    void operator++() { ++idx; }
    bool operator!=( citer const& rhs ) {
      return rhs.idx != idx;
    }
  };

  iter  begin() { return iter{this, 0}; }
  iter  end() { return iter{this, count()}; }
  citer begin() const { return citer{this, 0}; }
  citer end() const { return citer{this, count()}; }
};

class ViewVector : public CompositeView {
public:
  ViewVector() {}

  ViewVector( std::vector<OwningPositionedView> views )
    : views_( std::move( views ) ) {}

  // Implement CompositeView
  PositionedViewConst at_const( int idx ) const override;
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
  OneLineStringView( std::string msg, Color color, bool shadow );

  // Implement Object
  void draw( Texture const& tx, Coord coord ) const override;
  // Implement Object
  Delta delta() const override { return tx_.size(); }

  std::string const& msg() const { return msg_; }

protected:
  std::string msg_;
  Texture     tx_;
};

/****************************************************************
** Derived Views
*****************************************************************/
enum class e_option_active { inactive, active };

class OptionSelectItemView : public CompositeView {
public:
  OptionSelectItemView( std::string msg, W width );

  // Implement CompositeView
  PositionedViewConst at_const( int idx ) const override;
  // Implement CompositeView
  int count() const override { return 2; }

  void set_active( e_option_active active ) { active_ = active; }

  std::string const& line() const {
    return foreground_active_.msg();
  }

private:
  e_option_active   active_;
  SolidRectView     background_active_;
  SolidRectView     background_inactive_;
  OneLineStringView foreground_active_;
  OneLineStringView foreground_inactive_;
};

class OptionSelectView : public ViewVector {
public:
  OptionSelectView( StrVec const& options, W width,
                    int initial_selection );

  bool accept_input( input::event_t const& event ) override;

  std::string const& get_selected() const;
  bool               confirmed() const { return has_confirmed; }

private:
  ObserverPtr<OptionSelectItemView>  get_view( int item );
  ObserverCPtr<OptionSelectItemView> get_view( int item ) const;
  void                               set_selected( int item );

  int  selected_;
  bool has_confirmed;
};

/****************************************************************
** WindowManager
*****************************************************************/
using FinishedFunc = std::function<bool( void )>;

enum class e_window_state { running, closed };
enum class e_wm_input_result { unhandled, handled, quit };

class WindowManager {
public:
  void draw_layout( Texture const& tx ) const;

  void run( FinishedFunc const& finished_fn );

  ND e_wm_input_result accept_input( SDL_Event const& event );

  void add_window( std::string title, std::unique_ptr<View> view,
                   Coord position );

private:
  struct window {
    window( std::string title_, std::unique_ptr<View> view_,
            Coord position_ );

    void  draw( Texture const& tx ) const;
    Delta delta() const;
    Rect  rect() const;
    Coord inside_border() const;
    Rect  title_bar() const;

    e_window_state                     window_state;
    std::string                        title;
    std::unique_ptr<View>              view;
    std::unique_ptr<OneLineStringView> title_view;
    Coord                              position;
  };

  // Gets the window with focus, throws if no windows.
  window& focused();

  std::vector<window> windows_;
};

/****************************************************************
** High-level Methods
*****************************************************************/
std::string select_box( std::string const& title,
                        StrVec             options );

// TODO: create bimap and reference through type traits.
template<typename Enum>
Enum select_box_enum(
    std::string const&                        title,
    std::vector<std::pair<Enum, std::string>> options ) {
  // map over member function?
  std::vector<std::string> words;
  for( auto const& option : options )
    words.push_back( option.second );
  auto result = select_box( title, words );
  for( auto const& option : options )
    if( result == option.second ) return option.first;
  SHOULD_NOT_BE_HERE;
  return {};
}

enum class e_confirm { no, yes };

e_confirm yes_no( std::string const& title );

void message_box( std::string_view msg );

} // namespace rn::ui
