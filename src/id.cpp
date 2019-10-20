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
#include "fb.hpp"
#include "lua.hpp"

// Flatbuffers
#include "fb/sg-id_generated.h"

namespace rn {

namespace {

constexpr int kFirstUnitId = 1;

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( Id ) {
  SG_Id() : next_unit_id{kFirstUnitId - 1} {}

  // This will be called after deserialization.
  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }

  SAVEGAME_MEMBERS( Id, ( int, next_unit_id ) );

private:
  SAVEGAME_FRIENDS( Id );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).
    return xp_success_t{};
  }
};
SAVEGAME_IMPL( Id );

} // namespace

/****************************************************************
** Public Interface
*****************************************************************/
UnitId next_unit_id() {
  ++SG().next_unit_id;
  return UnitId{SG().next_unit_id};
}

/****************************************************************
** Testing
*****************************************************************/
namespace testing_only {
// FIXME: get rid of this.
void reset_unit_ids() { SG().next_unit_id = kFirstUnitId - 1; }
} // namespace testing_only

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( last_unit_id, UnitId ) {
  CHECK( SG().next_unit_id >= 0, "no units yet created." );
  return UnitId{SG().next_unit_id};
}

} // namespace

} // namespace rn
