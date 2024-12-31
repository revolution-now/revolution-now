/****************************************************************
**land-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-22.
*
* Description: Save-game state for the land view.
*
*****************************************************************/
#include "land-view.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

base::valid_or<string> Viewport::validate() const {
  REFL_VALIDATE( zoom >= 0.0, "zoom must be larger than zero" );
  REFL_VALIDATE( zoom >= 0.0, "zoom must be less than one" );
  REFL_VALIDATE( center_x >= 0.0,
                 "x center must be larger than 0" );
  REFL_VALIDATE( center_y >= 0.0,
                 "y center must be larger than 0" );
  return base::valid;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// LandViewState
LUA_STARTUP( lua::state& st ) {
  // Viewport
  [&] {
    using U = ::rn::Viewport;
    auto u  = st.usertype.create<U>();

    u["zoom"]     = &U::zoom;
    u["center_x"] = &U::center_x;
    u["center_y"] = &U::center_y;
  }();

  // LandViewState.
  [&] {
    using U = ::rn::LandViewState;
    auto u  = st.usertype.create<U>();

    u["viewport"] = &U::viewport;

    u["reveal_complete_map"] = []( U& o ) {
      // NOTE: need to redraw map after this.
      o.map_revealed = MapRevealed::entire{};
    };
  }();
};

} // namespace

} // namespace rn
