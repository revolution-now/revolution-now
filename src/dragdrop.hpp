/****************************************************************
**dragdrop.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-20.
*
* Description: A framework for drag & drop of entities.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "error.hpp"
#include "input.hpp"

// gfx
#include "gfx/coord.hpp"

// base
#include "base/scope-exit.hpp"

namespace rn::drag {

enum class e_status_indicator { none, bad, good };

struct Step {
  input::mod_keys mod;
  Coord           current;
};

template<typename DraggableObject>
struct State {
  // Input state.
  // FIXME: this can be removed when dragging is implemented the
  // way it is in the colony view.
  co::finite_stream<Step> stream;

  // Output state.
  DraggableObject    object;
  e_status_indicator indicator;
  bool               user_requests_input;
  Coord              where;
  Delta              click_offset;
};

} // namespace rn::drag
