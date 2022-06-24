/****************************************************************
**view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-12.
*
* Description: View base class.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "ui.hpp"

// gfx
#include "gfx/coord.hpp"

// C++ standard library
#include <memory>

namespace rn::ui {

class View : public Object {};

// This is a View coupled with a coordinate representing the po-
// sition of the upper-left corner of the view. Note that the co-
// ordinate is in the coordinate system of the parent view or
// window (whose position in turn will not be know by this
// struct).
struct PositionedView {
  View* view;
  Coord coord;

  Rect rect() const { return view->rect( coord ); }
};
NOTHROW_MOVE( PositionedView );
struct PositionedViewConst {
  View const* view;
  Coord       coord;

  Rect rect() const { return view->rect( coord ); }
};
NOTHROW_MOVE( PositionedViewConst );

// Same as above, but owns the view.  The
class OwningPositionedView {
 public:
  OwningPositionedView( std::unique_ptr<View> view,
                        Coord const&          coord )
    : view_( std::move( view ) ), coord_( coord ) {}

  View* view() const { return view_.get(); }
  View* view() { return view_.get(); }

  std::unique_ptr<View>& mutable_view() { return view_; }

  Coord const& coord() const { return coord_; }
  Coord&       coord() { return coord_; }

 private:
  std::unique_ptr<View> view_;
  Coord                 coord_;
};
NOTHROW_MOVE( OwningPositionedView );

} // namespace rn::ui
