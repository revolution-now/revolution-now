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
#include "fb.hpp"
#include "fmt-helper.hpp"
#include "id.hpp"
#include "orders.hpp"
#include "sg-macros.hpp"

// Flatbuffers
#include "fb/land-view_generated.h"

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( LandView );

struct UnitInputResponse {
  bool operator==( UnitInputResponse const& rhs ) const {
    return id == rhs.id && orders == rhs.orders &&
           add_to_front == rhs.add_to_front &&
           add_to_back == rhs.add_to_back;
  }
  bool operator!=( UnitInputResponse const& rhs ) const {
    return !( *this == rhs );
  }

  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, UnitInputResponse,
  ( UnitId,        id ),
  ( Opt<orders_t>, orders ),
  ( Vec<UnitId>,   add_to_front ),
  ( Vec<UnitId>,   add_to_back ));
  // clang-format on
};

struct Plane;
Plane* land_view_plane();

/****************************************************************
** Testing
*****************************************************************/
void test_land_view();

} // namespace rn

DEFINE_FORMAT( ::rn::UnitInputResponse,
               "UnitInputResponse{{id={},orders={},add_to_front="
               "{},add_to_back={}}}",
               o.id, o.orders,
               ::rn::FmtJsonStyleList{ o.add_to_front },
               ::rn::FmtJsonStyleList{ o.add_to_back } );
