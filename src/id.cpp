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
#include "sg-macros.hpp"

// Flatbuffers
#include "fb/sg-id_generated.h"

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( Id );

namespace {

constexpr int kFirstUnitId   = 1;
constexpr int kFirstColonyId = 1;

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( Id ) {
  SG_Id()
    : next_unit_id{ kFirstUnitId - 1 },
      next_colony_id{ kFirstColonyId - 1 } {}

  // This will be called after deserialization.
  valid_deserial_t check_invariants_safe() const {
    return valid;
  }

  // clang-format off
  SAVEGAME_MEMBERS( Id,
    ( int, next_unit_id   ),
    ( int, next_colony_id )
  );
  // clang-format on

private:
  SAVEGAME_FRIENDS( Id );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).
    return valid;
  }
  // Called after all modules are deserialized.
  SAVEGAME_VALIDATE() { return valid; }
};
SAVEGAME_IMPL( Id );

} // namespace

/****************************************************************
** Public Interface
*****************************************************************/
UnitId next_unit_id() {
  ++SG().next_unit_id;
  return UnitId{ SG().next_unit_id };
}

ColonyId next_colony_id() {
  ++SG().next_colony_id;
  return ColonyId{ SG().next_colony_id };
}

UnitId last_unit_id() {
  CHECK( SG().next_unit_id >= 0, "no units yet created." );
  return UnitId{ SG().next_unit_id };
}

ColonyId last_colony_id() {
  CHECK( SG().next_colony_id >= 0, "no colonies yet created." );
  return ColonyId{ SG().next_colony_id };
}

/****************************************************************
** Testing
*****************************************************************/
namespace testing_only {
// FIXME: get rid of this.
void reset_all_ids() {
  SG().next_unit_id   = kFirstUnitId - 1;
  SG().next_colony_id = kFirstColonyId - 1;
}
} // namespace testing_only

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( last_unit_id, UnitId ) { return last_unit_id(); }
LUA_FN( last_colony_id, ColonyId ) { return last_colony_id(); }

} // namespace

} // namespace rn
