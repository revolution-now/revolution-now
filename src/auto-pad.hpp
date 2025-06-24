/****************************************************************
**auto-pad.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-04-05.
*
* Description: Auto merged padding around UI elements.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "views.hpp"

// gfx
#include "gfx/coord.hpp"

// C++ standard library
#include <vector>

namespace rn {

// This algorithm asks each view if it needs padding around it
// using the `needs_padding` interface method. If yes then a view
// will receive half the requested padding, with the assumption
// that visually it will be situated next to another view that
// also needs padding (by the same amount), or will be sitting at
// the window's edge.
//
// The view's can_pad_immediate_children attribute will be re-
// spected; this can be used to turn off padding between the im-
// mediate children of a view (but does not apply around the in-
// side border of the view).
void autopad( std::unique_ptr<ui::View>& view );

} // namespace rn
