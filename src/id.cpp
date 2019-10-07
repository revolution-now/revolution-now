/****************************************************************
**id.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles IDs.
*
*****************************************************************/
#include "id.hpp"

// Revolution Now
#include "lua.hpp"

namespace rn {

namespace {

constexpr int kFirstUnitId = 1;

int g_next_unit_id{kFirstUnitId - 1};

} // namespace

UnitId next_unit_id() {
  ++g_next_unit_id;
  return UnitId{g_next_unit_id};
}

/****************************************************************
** Testing
*****************************************************************/
namespace testing_only {
void reset_unit_ids() { g_next_unit_id = kFirstUnitId - 1; }
} // namespace testing_only

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( last_unit_id, UnitId ) {
  CHECK( g_next_unit_id >= 0, "no units yet created." );
  return UnitId{g_next_unit_id};
}

} // namespace

} // namespace rn
