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
#include "coord.hpp"
#include "views.hpp"

// C++ standard library
#include <vector>

namespace rn {

// There are two methods for auto-padding, "fancy" and "not fan-
// cy." They are quite different from each other, though are
// supposed to produce the same results (but this is hard to
// reason about).
//
// The "fancy" one simply inserts padding around all elements in
// the view but then employs a complicated algorithm to collapse
// adjacent padding into a single padding (e.g. if a borderless
// view gets padded along with its contained view then there will
// appear to be too much padding; the fancy algorithm detects
// this and collapses it).
//
// The "not fancy" algorithm on the other hand basically just
// asks each view if it needs padding around it using the
// `needs_padding` interface method. If yes then a view will re-
// ceive half the requested padding, with the assumption that vi-
// sually it will be situated next to another view that also
// needs padding (by the same amount), or will be sitting at the
// window's edge.
//
// Both algorithms repsect the view's attribute called
// "can_pad_immediate_children" that can be used to turn off
// padding between the immediate children of a view (but does not
// apply around the inside border of the view).
//
// Each new window that is created should be tested with both to
// see if they produce similiar results. Then, if the "not fancy"
// turns out to suffice then the "fancy" one should be deleted,
// because the code is much more complex.
//
// Note that it is not a question of performance as to which to
// choose or keep, it is just correctness and code complexity.
void autopad( UPtr<ui::View>& view, bool use_fancy );
void autopad( UPtr<ui::View>& view, bool use_fancy, int pixels );

void test_autopad();

} // namespace rn
