/****************************************************************
**views.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-03-16.
*
* Description: Views for populating windows in the UI.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "color.hpp"
#include "sdl-util.hpp"
#include "ui.hpp"

namespace rn::ui {

// NOTE: this header should only be directly included by
// window.cpp.

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

  bool input( input::event_t const& event ) override;

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
  OptionSelectItemView( std::string msg );

  // Implement CompositeView
  PositionedViewConst at_const( int idx ) const override;
  // Implement CompositeView
  int count() const override { return 2; }

  void set_active( e_option_active active ) { active_ = active; }

  std::string const& line() const {
    return foreground_active_.msg();
  }

  void grow_to( W w );

private:
  e_option_active   active_;
  SolidRectView     background_active_;
  SolidRectView     background_inactive_;
  OneLineStringView foreground_active_;
  OneLineStringView foreground_inactive_;
};

class OptionSelectView : public ViewVector {
public:
  OptionSelectView( Vec<Str> const& options,
                    int             initial_selection );

  bool input( input::event_t const& event ) override;

  std::string const& get_selected() const;
  bool               confirmed() const { return has_confirmed; }

  void grow_to( W w );

private:
  ObserverPtr<OptionSelectItemView>  get_view( int item );
  ObserverCPtr<OptionSelectItemView> get_view( int item ) const;
  void                               set_selected( int item );

  int  selected_;
  bool has_confirmed;
};

} // namespace rn::ui
