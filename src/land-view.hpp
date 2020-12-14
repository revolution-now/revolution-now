/****************************************************************
**land-view.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-29.
*
* Description: Handles the main game view of the land.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "fmt-helper.hpp"
#include "id.hpp"
#include "orders.hpp"
#include "sync-future.hpp"

namespace rn {

struct UnitInputResponse {
  bool operator==( UnitInputResponse const& rhs ) const {
    return id == rhs.id && orders == rhs.orders &&
           add_to_front == rhs.add_to_front &&
           add_to_back == rhs.add_to_back;
  }
  bool operator!=( UnitInputResponse const& rhs ) const {
    return !( *this == rhs );
  }

  UnitId        id;
  Opt<orders_t> orders;
  Vec<UnitId>   add_to_front;
  Vec<UnitId>   add_to_back;
};

enum class e_depixelate_anim { none, death, demote };

void landview_ensure_unit_visible( UnitId id );

sync_future<UnitInputResponse> landview_ask_orders( UnitId id );

sync_future<> landview_animate_move( UnitId      id,
                                     e_direction direction );
sync_future<> landview_animate_attack(
    UnitId attacker, UnitId defender, bool attacker_wins,
    e_depixelate_anim dp_anim );

struct Plane;
Plane* land_view_plane();

/****************************************************************
** Testing
*****************************************************************/
void test_land_view();

} // namespace rn

DEFINE_FORMAT( rn::UnitInputResponse,
               "UnitInputResponse{{id={},orders={},add_to_front="
               "{},add_to_back={}}}",
               o.id, o.orders,
               rn::FmtJsonStyleList{ o.add_to_front },
               rn::FmtJsonStyleList{ o.add_to_back } );
