/****************************************************************
**rpt.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-20.
*
* Description: Implementation of the Recruit/Purchase/Train
*              buttons in the harbor view.
*
*****************************************************************/
#include "rpt.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "igui.hpp"
#include "ts.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> click_recruit( SS& ss, TS& ts, Player& player ) {
  co_await ts.gui.message_box( "Clicked Recruit." );
}

wait<> click_purchase( SS& ss, TS& ts, Player& player ) {
  co_await ts.gui.message_box( "Clicked Purchase." );
}

wait<> click_train( SS& ss, TS& ts, Player& player ) {
  co_await ts.gui.message_box( "Clicked Train." );
}

} // namespace rn
