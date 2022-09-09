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
struct OwningPositionedView {
  std::unique_ptr<View> view;
  Coord                 coord;

  Rect rect() const { return view->rect( coord ); }
};

} // namespace rn::ui
