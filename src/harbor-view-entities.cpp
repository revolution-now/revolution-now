/****************************************************************
**harbor-view-entities.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-08.
*
* Description: The various UI sections/entities in the european
*              harbor view.
*
*****************************************************************/
#include "harbor-view-entities.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

// Use this as the vtable key function.
void HarborSubView::update_this_and_children() {}

/****************************************************************
** HarborViewComposited
*****************************************************************/
void HarborViewComposited::update() {
  top_level->update_this_and_children();
}

/****************************************************************
** Public API
*****************************************************************/
HarborViewComposited recomposite_harbor_view(
    SS& ss, TS& ts, Player& player, Delta const& canvas_size ) {
  (void)ss;
  (void)ts;
  (void)player;
  (void)canvas_size;
  return {};
}

} // namespace rn
